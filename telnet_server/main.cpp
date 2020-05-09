#include "filter_utils.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/process/child.hpp>
#include <boost/process/async.hpp>
#include <boost/process/async_pipe.hpp>
#include <boost/process/io.hpp>

#include <iostream>
#include <mutex>
#include <vector>
#include <optional>
#include <charconv>

template<typename... Ts>
std::string format(Ts&&... args)
{
    std::ostringstream out;
    (out << ... << std::forward<Ts>(args));
    return out.str();
}

class logger
{
public:
    template<typename T>
    logger& operator<<(T&& output)
    {
        std::cerr << std::forward<T>(output);
        return *this;
    }
    ~logger()
    {
        std::cerr << std::endl;
    }
private:
    static std::mutex m_;
    std::lock_guard<std::mutex> l_{m_};
};

template<typename T>
std::optional<T> from_chars(std::string_view sv_) noexcept
{
    T out;
    auto end = sv_.data() + sv_.size();
    auto res = std::from_chars(sv_.data(), end, out);
    if(res.ec==std::errc{}&&res.ptr==end)
        return out;
    return {};
}

std::mutex logger::m_;

int main(int argc, char* argv[2])
{
    try {
        if(argc!=3)
            throw std::runtime_error(format("Usage: ", argv[0], " <listen-port> ", "<buffer-size>"));
        boost::asio::io_context ctx;
        boost::asio::signal_set stop_signals{ctx, SIGINT, SIGTERM};
        stop_signals.async_wait([&](boost::system::error_code ec, int /*signal*/){
            if(ec)
                return;
            logger{} << "Terminating in response to signal.";
            ctx.stop();
        });

        auto port = from_chars<std::uint16_t>(argv[1]);
        if(!port || !*port)
            throw std::runtime_error("Port must me in [1;65535]");

        auto limit = from_chars<std::size_t>(argv[2]);
        if(!port || !*port)
            throw std::runtime_error(format("Buffer size must be in [1;", std::numeric_limits<std::size_t>::max, "]"));

        boost::asio::spawn(ctx, [&ctx, port=*port, limit=*limit](boost::asio::yield_context yc){
            boost::asio::ip::tcp::acceptor acceptor{ctx};
            boost::asio::ip::tcp::endpoint ep{boost::asio::ip::tcp::v4(), port};
            acceptor.open(ep.protocol());
            acceptor.bind(ep);
            acceptor.listen();
            logger{} << "Listening on port " << port << "...";
            for(;;){
                boost::asio::ip::tcp::socket socket{boost::asio::make_strand(acceptor.get_executor())};
                boost::system::error_code ec;
                acceptor.async_accept(socket, yc[ec]);
                if(ec==boost::asio::error::operation_aborted)
                    return;
                if(ec)
                    logger{} << "Failed to accept connection: " << ec.message();
                else{
                    logger{} << "Client connected";
                    boost::asio::spawn(socket.get_executor(),
                                       [&ctx, socket=boost::asio::ip::tcp::socket{std::move(socket)}, limit]
                                       (boost::asio::yield_context yc) mutable {                                                
                        boost::system::error_code ec;
                        std::shared_ptr<boost::asio::ip::tcp::socket> socket_p =
                                std::make_shared<boost::asio::ip::tcp::socket>(std::move(socket));

                        boost::asio::signal_set sig_child_signal(ctx, SIGPIPE);
                        sig_child_signal.async_wait([&](boost::system::error_code ec, int /*signal*/){
                            if(ec)
                                return;
                            logger{} << "SIGPIPE";
                        });
                        boost::process::async_pipe in{ctx};
                        std::shared_ptr<boost::process::async_pipe> out_p = std::make_shared<boost::process::async_pipe>(ctx);
                        boost::process::child c(ctx, "bash",
                                                boost::process::std_in < in,
                                                (boost::process::std_out & boost::process::std_err) > *out_p);

                        std::mutex output_m_;

                        boost::asio::spawn(ctx, [socket_p, out_p, limit, &output_m_](boost::asio::yield_context yc) {
                            boost::system::error_code ec;
                            std::string out_buf;
                            out_buf.resize(limit*2);
                            for(;;){
                                std::size_t str_size = out_p->async_read_some(boost::asio::buffer(out_buf, limit), yc[ec]);
                                if(ec){
                                    logger{} << "out async_read: " << ec.message();
                                    return;
                                }
                                add_r(out_buf, str_size);
                                {
                                    std::lock_guard<std::mutex> l_{output_m_};
                                    boost::asio::async_write(*socket_p, boost::asio::buffer(out_buf, str_size), yc[ec]);
                                }
                                if(ec){
                                    logger{} << "socket async_send: " << ec.message();
                                    return;
                                }
                            }
                        });

                        std::string in_buf;
                        in_buf.resize(limit);
                        std::size_t str_size;

                        auto filter_commands = [&]() mutable {
                            std::string::size_type iac_pos;
                            while((str_size > 0) &&
                                  (iac_pos = in_buf.find_first_of(code_bytes::IAC)) != std::string::npos){
                                std::string::size_type cmd_pos = iac_pos+1;
                                if(cmd_pos >= str_size){
                                    char two_bytes[2];
                                    boost::asio::async_read(*socket_p, boost::asio::buffer(two_bytes, 2), yc[ec]);
                                    if(ec)
                                        return;
                                    in_buf.append(two_bytes, 2);
                                    str_size+=2;
                                }
                                char cmd = in_buf[cmd_pos];
                                if(cmd >= code_bytes::WILL && cmd <= code_bytes::DONT){//DO, DONT, WILL< WONT
                                    char opt;
                                    if((cmd_pos + 1) >= str_size){
                                        char one_byte[1];
                                        boost::asio::async_read(*socket_p, boost::asio::buffer(one_byte, 1), yc[ec]);
                                        if(ec)
                                            return;
                                        in_buf.append(one_byte, 1);
                                        str_size+=1;
                                        return;
                                    }
                                    opt = in_buf[cmd_pos+1];
                                    char o[3] = {code_bytes::IAC, 0, opt};
                                    switch(opt){
                                    case code_bytes::DO:
                                    {   std::lock_guard<std::mutex> l_{output_m_};
                                        o[1] = code_bytes::WONT;
                                        boost::asio::async_write(*socket_p, boost::asio::buffer(o, 3), yc[ec]);
                                    }
                                        break;
                                    case code_bytes::WILL:
                                    {   std::lock_guard<std::mutex> l_{output_m_};
                                        o[1] = code_bytes::DONT;
                                        boost::asio::async_write(*socket_p, boost::asio::buffer(o, 3), yc[ec]);
                                    }
                                        break;
                                    case code_bytes::WONT:
                                    {   std::lock_guard<std::mutex> l_{output_m_};
                                        o[1] = code_bytes::DONT;
                                        boost::asio::async_write(*socket_p, boost::asio::buffer(o, 3), yc[ec]);
                                    }
                                        break;
                                    default: //DONT
                                        break;
                                    }
                                    in_buf.erase(iac_pos, 3);
                                    str_size -=3;
                                } else {
                                    switch(cmd){
                                    case code_bytes::IP:
                                        kill(c.id(), SIGINT);
                                        break;
                                    case code_bytes::AYT:
                                    {
                                        std::string i_am_there("Server is online...\n");
                                        std::lock_guard<std::mutex> l_{output_m_};
                                        boost::asio::async_write(*socket_p,
                                                                 boost::asio::buffer(i_am_there, i_am_there.size()), yc[ec]);
                                    }
                                        break;
                                    default:
                                        break;
                                    }
                                    in_buf.erase(iac_pos, 2);
                                    str_size -=2;
                                }
                            }
                        };

                        for(;;){
                            str_size = socket_p->async_read_some(boost::asio::buffer(in_buf, limit), yc[ec]);
                            if(ec){
                                logger{} << "socket async_read: " << ec.message();
                                socket.shutdown(boost::asio::ip::tcp::socket::shutdown_receive, ec);
                                break;
                            }
                            filter_commands();
                            filter_r(in_buf, str_size);
                            boost::asio::async_write(in, boost::asio::buffer(in_buf, str_size), yc[ec]);
                            if(ec){
                                logger{} << "in async_write: " << ec.message();
                                in.close();
                                break;
                            }
                        }
                        if(ec)
                            logger{} << ec.message();
                    });
                }
            }
        });
        ctx.run();
    } catch (...) {
        logger{} << boost::current_exception_diagnostic_information();
    }

    return 0;
}

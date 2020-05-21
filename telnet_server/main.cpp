#include "filter_utils.hpp"
#include "pseudo_terminal.hpp"

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
#include <atomic>
#include <list>
#include <thread>

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

class write_query
{
public:
    write_query() : on_writing_(false), l_{} {};
    void write(std::shared_ptr<boost::asio::ip::tcp::socket> socket_p, std::string s,
               boost::asio::yield_context & yc, boost::system::error_code & ec)
    {
        {
            std::lock_guard lg(list_access_);
            l_.push_back(std::move(s));
        }
        bool exchanged, expected(false);
        exchanged = on_writing_.compare_exchange_weak(expected, true);
        if(exchanged){
            while(true){
                {
                    std::lock_guard lg(list_access_);
                    if(l_.empty()){
                        on_writing_.store(false);
                        break;
                    }
                    s = std::move(l_.front());
                    l_.pop_front();
                }
                boost::asio::async_write(*socket_p, boost::asio::buffer(s, s.size()), yc[ec]);
                if(ec){
                    logger{} << "Write query: " << ec.message();
                }
            }
        }
    }
private:
    std::atomic_bool on_writing_;
    std::mutex list_access_;
    std::list<std::string> l_;
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

int main(int argc, char* argv[4])
{
    try {
        if(argc < 3 || argc > 4)
            throw std::runtime_error(format("Usage: ", argv[0], " <listen-port> ", "<buffer-size> ", "<thread-number>"));
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

        std::optional<std::uint32_t> threads;
        if(argc==4){
            threads = from_chars<std::uint32_t>(argv[3]);
            if(!threads || !*threads || *threads > std::thread::hardware_concurrency())
            throw std::runtime_error(format("Number of threads should be in [1;", std::thread::hardware_concurrency()));
        } else
            threads = std::thread::hardware_concurrency();

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

                        std::shared_ptr<write_query> wq_p = std::make_shared<write_query>();

                        boost::asio::signal_set sig_child_signal(ctx, SIGCHLD);
                        sig_child_signal.async_wait([&](boost::system::error_code ec, int /*signal*/){
                            if(ec)
                                return;
                            logger{} << "SIGCHLD";
                        });  

                        std::shared_ptr<pty> pterminal_p = std::make_shared<pty>(ctx);
                        boost::process::child c(ctx, "bash -i -s -l",
                                                boost::process::std_in < pterminal_p->pslave_name(),
                                                (boost::process::std_out & boost::process::std_err) > pterminal_p->pslave_name());

                        boost::asio::spawn(ctx, [socket_p, wq_p, pterminal_p, limit](boost::asio::yield_context yc) {
                            boost::system::error_code ec;
                            std::string out_buf;
                            out_buf.resize(limit*2);
                            for(;;){
                                std::size_t str_size = pterminal_p->master.async_read_some(boost::asio::buffer(out_buf, limit), yc[ec]);
                                if(ec){
                                    logger{} << "master terminal async_read: " << ec.message();
                                    return;
                                }
                                add_r(out_buf, str_size);
                                wq_p->write(socket_p, out_buf, yc, ec);
                                out_buf.clear();
                                out_buf.resize(limit*2);
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
                                        o[1] = code_bytes::WONT;
                                        wq_p->write(socket_p, std::string(o), yc, ec);
                                        break;
                                    case code_bytes::WILL:
                                        o[1] = code_bytes::DONT;
                                        wq_p->write(socket_p, std::string(o), yc, ec);
                                        break;
                                    case code_bytes::WONT:
                                        o[1] = code_bytes::DONT;
                                        wq_p->write(socket_p, std::string(o), yc, ec);
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
                                        wq_p->write(socket_p, "Server is online...\n", yc, ec);
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
                            boost::asio::async_write(pterminal_p->master, boost::asio::buffer(in_buf, str_size), yc[ec]);
                            if(ec){
                                logger{} << "master terminal async_write: " << ec.message();
                                pterminal_p->master.close();
                                break;
                            }
                        }
                        if(ec)
                            logger{} << ec.message();
                    });
                }
            }
        });

        std::vector<std::thread> workers;
        workers.reserve(*threads-1);

        for(unsigned i=1; i<*threads; ++i){
            workers.emplace_back([&]{
                ctx.run();
            });
        }

        ctx.run();
    } catch (...) {
        logger{} << boost::current_exception_diagnostic_information();
    }

    return 0;
}

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
#include <boost/program_options.hpp>

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

std::mutex logger::m_;

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

struct telnet_session : std::enable_shared_from_this<telnet_session>
{
    telnet_session(boost::asio::io_context& ctx, boost::asio::ip::tcp::socket&& s)
        : socket(std::move(s)),
        pterminal(ctx),
        c(ctx, "bash -i -s -l",
          boost::process::std_in < pterminal.pslave_name(),
                                 (boost::process::std_out & boost::process::std_err) > pterminal.pslave_name())
    {}
    telnet_session(telnet_session&& other)
        : socket(std::move(other.socket)),
        pterminal(std::move(other.pterminal)),
        c(std::move(other.c))
    {}

    boost::asio::ip::tcp::socket socket;
    pty pterminal;
    boost::process::child c;
};

int main(int argc, char* argv[4])
{
    try {
        std::string port_str, thread_str, limit_str;
        boost::program_options::options_description desc("Allowed options");
        desc.add_options()
            ("help", "Produce this message")
                    ("port", boost::program_options::value<std::string>(&port_str)->default_value("23"), "specify port")
                        ("thread-number", boost::program_options::value<std::string>(&thread_str)->default_value("1"), "set the amount of working threads")
                        ("buffer-size", boost::program_options::value<std::string>(&limit_str)->default_value("2048"), "set the size of I/O buffers");
        boost::program_options::variables_map vm;
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
        boost::program_options::notify(vm);

        if(vm.count("help")) {
            std::cout << desc << std::endl;
            return 0;
        }

        auto port = from_chars<std::uint16_t>(port_str);
        if(!port || !*port)
            throw std::runtime_error("Port must me in [1;65535]");

        auto limit = from_chars<std::size_t>(limit_str);
        if(!limit || !*limit)
            throw std::runtime_error(format("Buffer size must be in [1;", std::numeric_limits<std::size_t>::max, "]"));

        auto threads = from_chars<std::uint32_t>(thread_str);
        if(!threads || !*threads || *threads > std::thread::hardware_concurrency())
            throw std::runtime_error(format("Number of threads should be in [1;", std::thread::hardware_concurrency(), "]\n"));

        std::cout << "Execution with the next parameters:"
                  << "\nport: " << port_str
                  << "\nnumber of threads: " << thread_str
                  << "\nbuffer size: " << limit_str << std::endl;


        boost::asio::io_context ctx;
        boost::asio::signal_set stop_signals{ctx, SIGINT, SIGTERM};
        stop_signals.async_wait([&](boost::system::error_code ec, int /*signal*/){
            if(ec)
                return;
            logger{} << "Terminating in response to signal.";
            ctx.stop();
        });

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
                    boost::asio::spawn(*socket.get_executor().target<boost::asio::strand<boost::asio::ip::tcp::socket::executor_type>>(),
                                       [&ctx, socket=std::move(socket), limit]
                                       (boost::asio::yield_context yc) mutable {                                                
                        boost::system::error_code ec;

                        std::shared_ptr<telnet_session> session_p = std::make_shared<telnet_session>(ctx, std::move(socket));

                        boost::asio::signal_set sig_child_signal(*session_p->socket.get_executor().target
                                                                  <boost::asio::strand<boost::asio::ip::tcp::socket::executor_type>>(),
                                                                 SIGCHLD);
                        sig_child_signal.async_wait([&](boost::system::error_code ec, int /*signal*/){
                            if(ec)
                                return;
                            session_p->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_receive, ec);
                            //session_p->c.terminate();
                            //session_p->pterminal.master.close();
                            logger{} << "SIGCHLD";
                        });  

                        boost::asio::spawn(*session_p->socket.get_executor().target<boost::asio::strand<boost::asio::ip::tcp::socket::executor_type>>(),
                                           [session_p, limit](boost::asio::yield_context yc) {
                            boost::system::error_code ec;
                            std::string out_buf;
                            out_buf.resize(limit*2);
                            for(;;){
                                std::size_t str_size = session_p->pterminal.master.async_read_some(boost::asio::buffer(out_buf, limit), yc[ec]);
                                if(ec){
                                    logger{} << "master terminal async_read: " << ec.message();
                                    session_p->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_receive, ec);
                                    //session_p->c.terminate();
                                    //session_p->pterminal.master.close();
                                    return;
                                }
                                //add_r(out_buf, str_size);
                                boost::asio::async_write(session_p->socket, boost::asio::buffer(out_buf, str_size), yc[ec]);
//                                out_buf.clear();
//                                out_buf.resize(limit*2);
                                if(ec){
                                    logger{} << "socket async_send: " << ec.message();
                                    session_p->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_receive, ec);
                                    //session_p->c.terminate();
                                    //session_p->pterminal.master.close();
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
                                    boost::asio::async_read(session_p->socket, boost::asio::buffer(two_bytes, 2), yc[ec]);
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
                                        boost::asio::async_read(session_p->socket, boost::asio::buffer(one_byte, 1), yc[ec]);
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
                                        boost::asio::async_write(session_p->socket, boost::asio::buffer(o), yc[ec]);
                                        break;
                                    case code_bytes::WILL:
                                        o[1] = code_bytes::DONT;
                                        boost::asio::async_write(session_p->socket, boost::asio::buffer(o), yc[ec]);
                                        break;
                                    case code_bytes::WONT:
                                        o[1] = code_bytes::DONT;
                                        boost::asio::async_write(session_p->socket, boost::asio::buffer(o), yc[ec]);
                                        break;
                                    default: //DONT
                                        break;
                                    }
                                    in_buf.erase(iac_pos, 3);
                                    str_size -=3;
                                } else {
                                    switch(cmd){
                                    case code_bytes::IP:
                                        kill(session_p->c.id(), SIGINT);
                                        break;
                                    case code_bytes::AYT:
                                        boost::asio::async_write(session_p->socket, boost::asio::buffer("Server is online"), yc[ec]);
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
                            str_size = session_p->socket.async_read_some(boost::asio::buffer(in_buf, limit), yc[ec]);
                            if(ec){
                                logger{} << "socket async_read: " << ec.message();
                                session_p->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_receive, ec);
                                //session_p->c.terminate();
                                //session_p->pterminal.master.close();
                                break;
                            }
                            filter_commands();
                            filter_r(in_buf, str_size);
                            boost::asio::async_write(session_p->pterminal.master, boost::asio::buffer(in_buf, str_size), yc[ec]);
                            if(ec){
                                logger{} << "master terminal async_write: " << ec.message();
                                session_p->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_receive, ec);
                                //session_p->c.terminate();
                                //session_p->pterminal.master.close();
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

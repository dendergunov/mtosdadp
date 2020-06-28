#include "event_wrappers.hpp"
#include "filter_utils.hpp"

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/thread.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <iostream>
#include <sstream>
#include <charconv>
#include <optional>
#include <thread>
#include <vector>
#include <chrono>

#include <boost/program_options.hpp>

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

void eventcb(struct bufferevent *bev, short events, void *ptr)
{
    if(events & BEV_EVENT_CONNECTED){
        static int total_connected = 0;
//        logger{} << "Sucessfully connected to telnet server! " << ++total_connected;
    } else if(events & BEV_EVENT_ERROR){
        throw std::runtime_error("Error connecting to server!");
    }
}

void readcb(struct bufferevent *bev, void *ptr)
{
    auto libev = static_cast<libevent::bufferevent_w*>(ptr);
    struct evbuffer *in = bufferevent_get_input(bev);
    std::size_t length = evbuffer_get_length(in);

    if(length){
        libev->bytes_read += length;
        evbuffer_drain(in, length);
    }
    if(libev->bytes_read >= libev->bytes_read_threshold){
        static int i = 0;
//        logger{} << "bytes_read: " << libev->bytes_read << ", threshold: " << libev->bytes_read_threshold
//                 << "\nbytes_send: "<< libev->bytes_send << ", threshold: " << libev->bytes_send_threshold
//                 << ", socket: " << ++i;
        libev->close();
    }
}

void writecb(struct bufferevent *bev, void *ptr)
{
    auto libev = static_cast<libevent::bufferevent_w*>(ptr);
    libev->bytes_send += libev->send_message.size();

    if(libev->bytes_send < libev->bytes_send_threshold){
        bufferevent_write(bev, libev->send_message.data(), libev->send_message.size());
    }
//        else {
//        logger{} << "bytes_send: " << libev->bytes_send;
//    }
}

//void mainreadcb(struct bufferevent *bev, void *ptr)
//{
//    auto libev = static_cast<libevent::bufferevent_w*>(ptr);

//    struct evbuffer *in = bufferevent_get_input(bev);
//    int length = evbuffer_get_length(in);

//    if(length){
//        libev->bytes_read += length;
////        evbuffer_drain(in, length);
//    }

//    std::string readbuf;
//    readbuf.resize(length);
//    int ec;
//    ec = bufferevent_read(bev, readbuf.data(), length);
//    logger{} << "Read " << ec << " bytes";
//    logger{} << "Total: " << libev->bytes_read;
//    std::cout << readbuf << std::flush;

//    if(libev->bytes_read >= libev->bytes_read_threshold){
//        logger{} << "bytes_read: " << libev->bytes_read << ", threshold: " << libev->bytes_read_threshold;
//        libev->close();
//    }
//}

//void stdinrcb(evutil_socket_t fd, short what, void* arg)
//{
////    logger{} << "srdinrcb!";
//    std::vector<libevent::bufferevent_w> *bev = static_cast<std::vector<libevent::bufferevent_w>*>(arg);
//    std::string input;
//    input.resize(2048);
//    std::cin.getline(input.data(), 2048);
//    input.append("\n");
//    auto size = input.size();
//    add_r(input, size);

//    auto bev_size = bev->size();
//    for(std::size_t i = 0; i < bev_size; ++i){
//        bufferevent_write((*bev)[i].bev, input.data(), size);
//    }
//}
std::string get_send_message(int policy, std::size_t send_byte_threshold, std::size_t read_byte_threshold);

int main(int argc, char **argv)
{
    try {
        std::string address_str, port_str, thread_str, connection_str, recieve_threshold_str, policy_str;

        boost::program_options::options_description desc("Allowed options");
        desc.add_options()
        ("help", "Produce this message")
        ("ip-address", boost::program_options::value<std::string>(&address_str)->default_value("127.0.0.1"), "specify host ip-address")
        ("port", boost::program_options::value<std::string>(&port_str)->default_value("23"), "specify port")
        ("thread-number", boost::program_options::value<std::string>(&thread_str)->default_value("1"), "set the amount of working threads")
        ("connection-number", boost::program_options::value<std::string>(&connection_str)->default_value("1"), "set the number of connections")
        ("recieve-threshold", boost::program_options::value<std::string>(&recieve_threshold_str)->default_value("4096"), "set the number of bytes to recieve "
                                                                                                                  "before socket shutdown")
        ("policy", boost::program_options::value<std::string>(&policy_str)->default_value("0"), "set 0 to send a little message to get a light answer "
                                                                                                     "or 1 to send a light message to get a heavy answer "
                                                                                                     "or 2 to send a heavy message to get a heavy answer");
        boost::program_options::variables_map vm;
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
        boost::program_options::notify(vm);

        if(vm.count("help")) {
            std::cout << desc << std::endl;
            return 0;
        }

        struct in_addr address;
        if(!inet_pton(AF_INET, address_str.data(), &address))
            throw std::runtime_error("Cannot convert ip address\n");

        auto port = from_chars<std::uint16_t>(port_str);
        if(!port || !*port)
            throw std::runtime_error("Port must be in [1;65535]\n");

        auto threads = from_chars<std::uint32_t>(thread_str);
        if(!threads || !*threads || *threads > std::thread::hardware_concurrency())
            throw std::runtime_error(format("Number of threads should be in [1;", std::thread::hardware_concurrency(), "]\n"));

        auto connections = from_chars<std::uint32_t>(connection_str);
        if(!connections || !*connections)
            throw std::runtime_error(format("Number of connections must be in [1;", std::numeric_limits<std::uint32_t>::max, "]\n"));

        auto read_byte_threshold = from_chars<std::size_t>(recieve_threshold_str);
        if(!read_byte_threshold || !*read_byte_threshold)
            throw std::runtime_error(format("Recieve threshold must be in [1;", std::numeric_limits<std::size_t>::max, "\n"));

        auto policy = from_chars<int>(policy_str);
        if(!policy)
            throw std::runtime_error(format("policy must be in [0;2]"));
        if(*policy < 0 || *policy > 2)
            throw std::runtime_error(format("Policy must be in [0;2]"));

        std::cout << "Execution with the next parameters:"
                  << "\nip-address: " << address_str
                  << "\nport: " << port_str
                  << "\nnumber of threads: " << thread_str
                  << "\nnumber of connections: " << connection_str
                  << "\nrecieve threshold: " << recieve_threshold_str
                  << "\ntransmission policy: " << policy_str << std::endl;

        /**----------------------------------------------------------------------------------**/

        struct sockaddr_in sin;
        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = address.s_addr;
        sin.sin_port = htons(*port);

        /**----------------------------------------------------------------------------------**/
        evthread_use_pthreads();

        std::vector<libevent::bufferevent_w> bevents;
        bevents.reserve(*connections);

        std::vector<libevent::event_base_w> evbases;
        evbases.reserve(*threads);
        for(std::size_t i = 0; i < *threads; ++i)
            evbases.emplace_back();

        std::vector<std::thread> workers;
        workers.reserve(*threads-1);

        std::size_t send_byte_threshold;
        if(*policy == 1){
            send_byte_threshold = 50;
        } else {
            send_byte_threshold = *read_byte_threshold;
        }

        std::string send_message = get_send_message(*policy, send_byte_threshold, *read_byte_threshold);

        for(std::size_t i = 0; i < *connections; ++i){
            bevents.emplace_back(libevent::bufferevent_w(evbases[i%evbases.size()], *read_byte_threshold,
                                                         send_byte_threshold, std::string_view(send_message.data(), send_message.size())));
//            if(i == 0)
//                bufferevent_setcb(bevents[i].bev, mainreadcb, NULL, eventcb, &bevents[i]);
//            else
                bufferevent_setcb(bevents[i].bev, readcb, writecb, eventcb, &bevents[i]);
            bufferevent_enable(bevents[i].bev, EV_READ|EV_WRITE);
        }

        auto start = std::chrono::high_resolution_clock::now();

        for(std::size_t i = 0; i < *connections; ++i){
            if(bufferevent_socket_connect(bevents[i].bev,(struct sockaddr *) &sin, sizeof(sin)) < 0){
                throw std::runtime_error(format("bufferevent_socket_connect error on ", i, " socket"));
                return -1;
            }
            bufferevent_write(bevents[i].bev, send_message.data(), send_message.size());
        }

        for(unsigned int i = 1; i < *threads; ++i){
            workers.emplace_back([&, index=i]{
                event_base_dispatch(evbases[index].base);
            });
        }

//        libevent::bufferevent_w stdcin(evbases[0], 0);
//        libevent::event_w stdreading;
//        stdreading.ev = event_new(evbases[0].base, 0, EV_READ | EV_PERSIST, stdinrcb, &bevents);

//        event_add(stdreading.ev, NULL);

        event_base_dispatch(evbases[0].base);

        for(auto& x: workers){
            x.join();
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end-start;

        logger{} << "Processed in " <<diff.count()
                 << "s, one client in: " << diff.count()/(*connections);

    }  catch (std::exception& e) {
        std::cerr << e.what();
    }

    return 0;
}

std::string get_send_message(int policy, std::size_t send_bytes_threshold, std::size_t read_byte_threshold)
{
    std::string message("light message");
    std::size_t size = message.size();
    switch(policy){
    case 0:
        return format("echo \"", message, "\"\n");
        break;
    case 1:
        return format("for ((i=1;i<=", read_byte_threshold/message.size()+1, ";i+=1)); do echo \"", message ,"$i\"; done\n");
    case 2:
        if(send_bytes_threshold < 1048576){
            for(std::size_t i = 0; i <= send_bytes_threshold/size; ++i)
                message.append("light message");
        } else {
            for(std::size_t i = 0; i <= 1048576/size; ++i)
                message.append("light message");
        }
        return format("echo \"", message, "\"\n");
    default:
        throw std::runtime_error(format("policy value ", policy, " cannot be accepted in get_send_message!"));
    }
}

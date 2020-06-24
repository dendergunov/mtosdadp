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
        logger{} << "Sucessfully connected to telnet server!";
    } else if(events & BEV_EVENT_ERROR){
        throw std::runtime_error(format("Error connecting to server!"));
    }
}

void readcb(struct bufferevent *bev, void *ptr)
{
    //logger{} << "Readcb";
    struct evbuffer *in = bufferevent_get_input(bev);
    int length = evbuffer_get_length(in);
    //logger{} << length;

    std::string readbuf;
    readbuf.resize(length);
    int ec;
    ec = bufferevent_read(bev, readbuf.data(), length);
    //logger{} << "Read " << ec << " bytes";
    std::cout << readbuf;
}

void writecb(struct bufferevent *bev, void *ptr)
{
    std::cout << "Writecb\n";

}

void stdinrcb(evutil_socket_t fd, short what, void* arg)
{
    logger{} << "srdinrcb!";
    std::vector<libevent::bufferevent_w> *bev = static_cast<std::vector<libevent::bufferevent_w>*>(arg);
    std::string input;
    input.resize(2048);
    std::cin.getline(input.data(), 2048);
    input.append("\n");
    auto size = input.size();
    add_r(input, size);

    auto bev_size = bev->size();
    for(std::size_t i = 0; i < bev_size; ++i){
        bufferevent_write((*bev)[i].bev, input.data(), size);
    }

}

int main(int argc, char **argv)
{
    try {
        std::string address_str, port_str, limit_str, thread_str, connection_str;

        boost::program_options::options_description desc("Allowed options");
        desc.add_options()
        ("help", "Produce this message")
        ("ip-address", boost::program_options::value<std::string>(&address_str)->default_value("127.0.0.1"), "specify host ip-address")
        ("port", boost::program_options::value<std::string>(&port_str)->default_value("23"), "specify port")
        ("buffer-limit", boost::program_options::value<std::string>(&limit_str)->default_value("2048"), "set I/O buffer in bytes")
        ("thread-number", boost::program_options::value<std::string>(&thread_str)->default_value("1"), "set the amount of working threads")
        ("connection-number", boost::program_options::value<std::string>(&connection_str)->default_value("1"), "set the number of connections");
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

        auto limit = from_chars<std::size_t>(limit_str);
        if(!limit || !*limit)
            throw std::runtime_error(format("Buffer size must be in [1;", std::numeric_limits<std::size_t>::max, "]\n"));

        auto threads = from_chars<std::uint32_t>(thread_str);
        if(!threads || !*threads || *threads > std::thread::hardware_concurrency())
            throw std::runtime_error(format("Number of threads should be in [1;", std::thread::hardware_concurrency(), "]\n"));

        auto connections = from_chars<std::uint32_t>(connection_str);
        if(!connections || !*connections)
            throw std::runtime_error(format("Number of connections must be in [1;", std::numeric_limits<std::uint32_t>::max, "]\n"));

        std::cout << "Execution with next parameters:"
                  << "\nip-address: " << address_str
                  << "\nport: " << port_str
                  << "\nnumber of threads: " << thread_str
                  << "\nnumber of connections: " << connection_str << std::endl;

        /**----------------------------------------------------------------------------------**/

        struct sockaddr_in sin;
        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = address.s_addr;
        sin.sin_port = htons(*port);

        /**----------------------------------------------------------------------------------**/

        std::vector<libevent::bufferevent_w> bevents;
        bevents.reserve(*connections);

        libevent::event_base_w evbase;

        for(std::size_t i = 0; i < *connections; ++i){
            bevents.emplace_back(libevent::bufferevent_w(evbase));
            bufferevent_setcb(bevents[i].bev, readcb, NULL, eventcb, NULL);
            bufferevent_enable(bevents[i].bev, EV_READ|EV_WRITE);
            if(bufferevent_socket_connect(bevents[i].bev,(struct sockaddr *) &sin, sizeof(sin)) < 0){
                throw std::runtime_error("bufferevent_socket_connect error");
                return -1;
            }
        }

        libevent::bufferevent_w stdcin(evbase);
        libevent::event_w stdreading;
        stdreading.ev = event_new(evbase.base, 0, EV_READ | EV_PERSIST, stdinrcb, &bevents);

        event_add(stdreading.ev, NULL);

        event_base_dispatch(evbase.base);
    }  catch (std::exception& e) {
        std::cerr << e.what();
    }

    return 0;
}

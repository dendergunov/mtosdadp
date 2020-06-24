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
    //logger{} << "srdinrcb!";
    struct bufferevent * bev = static_cast<struct bufferevent *>(arg);
    std::string input;
    input.resize(2048);
    std::cin.getline(input.data(), 2048);
    input.append("\n");
    auto size = input.size();
    add_r(input, size);

    bufferevent_write(bev, input.data(), size);

}

int main(int argc, char **argv)
{
    try {
        if(argc < 4 || argc > 5)
            throw std::runtime_error(format("Usage: ", argv[0], " <ip-address> <port> <buffer_size> <thread_number>\n"));

        struct in_addr address;
        if(!inet_pton(AF_INET, argv[1], &address))
            throw std::runtime_error("Cannot convert ip address\n");

        auto port = from_chars<std::uint16_t>(argv[2]);
        if(!port || !*port)
            throw std::runtime_error("Port must be in [1;65535]\n");

        auto limit = from_chars<std::size_t>(argv[3]);
        if(!limit || !*limit)
            throw std::runtime_error(format("Buffer size must be in [1;", std::numeric_limits<std::size_t>::max, "]\n"));

        std::optional<std::uint32_t> threads;
        if(argc==5){
            threads = from_chars<std::uint32_t>(argv[4]);
            if(!threads || !*threads || *threads > std::thread::hardware_concurrency())
                throw std::runtime_error(format("Number of threads should be in [1;", std::thread::hardware_concurrency(), "]\n"));
        } else
            threads = std::thread::hardware_concurrency();
        /**----------------------------------------------------------------------------------**/

        struct sockaddr_in sin;
        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = address.s_addr;
        sin.sin_port = htons(*port);

        /**----------------------------------------------------------------------------------**/

        libevent::event_base_w evbase;
        libevent::bufferevent_w bev(evbase);


        bufferevent_setcb(bev.bev, readcb, NULL, eventcb, NULL);
        bufferevent_enable(bev.bev, EV_READ|EV_WRITE);

        if(bufferevent_socket_connect(bev.bev,(struct sockaddr *) &sin, sizeof(sin)) < 0){
            throw std::runtime_error("bufferevent_socket_connect error");
            return -1;
        }

        libevent::bufferevent_w stdcin(evbase);

        libevent::event_w stdreading;
        stdreading.ev = event_new(evbase.base, 0, EV_READ | EV_PERSIST, stdinrcb, bev.bev);

        event_add(stdreading.ev, NULL);

        event_base_dispatch(evbase.base);
    }  catch (std::exception& e) {
        std::cerr << e.what();
    }

    return 0;
}

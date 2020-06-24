#include "event_wrappers.hpp"

#include <event2/event.h>
#include <event2/bufferevent.h>
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
        std::cout << "CONNECTED KEKW\n";
    } else if(events & BEV_EVENT_ERROR){
        throw std::runtime_error("Error connecting to 127.0.0.1:8080");
    }
}

int main(int argc, char **argv)
{
    try {
        if(argc < 4 || argc > 5)
            throw std::runtime_error(format("Usage: ", argv[0], " <ip-address> <port> <buffer_size> <thread_number>\n"));

        sa_family_t address;
        if(!inet_pton(AF_INET, argv[1], &address))
            throw std::runtime_error("Cannot convert ip address\n");

        auto port = from_chars<std::uint16_t>(argv[2]);
        if(!port || !*port)
            throw std::runtime_error("Port must me in [1;65535]\n");

        auto limit = from_chars<std::size_t>(argv[3]);
        if(!port || !*port)
            throw std::runtime_error(format("Buffer size must be in [1;", std::numeric_limits<std::size_t>::max, "]\n"));

        std::optional<std::uint32_t> threads;
        if(argc==5){
            threads = from_chars<std::uint32_t>(argv[4]);
            if(!threads || !*threads || *threads > std::thread::hardware_concurrency())
                throw std::runtime_error(format("Number of threads should be in [1;", std::thread::hardware_concurrency(), "]\n"));
        } else
            threads = std::thread::hardware_concurrency();
        /**----------------------------------------------------------------------------------**/

        struct event_base *base;
        struct bufferevent *bev;
        struct sockaddr_in sin;

        base = event_base_new();
        if(!base)
            throw std::runtime_error("Can't create event_base!");

        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = htonl(0x7f000001);
        sin.sin_port = htons(8080);

        bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);

        bufferevent_setcb(bev, NULL, NULL, eventcb, NULL);

        if(bufferevent_socket_connect(bev,(struct sockaddr *) &sin, sizeof(sin)) < 0){
            bufferevent_free(bev);
            event_base_free(base);
            throw std::runtime_error("bufferevent_socket_connect error");
            return -1;
        }

        event_base_dispatch(base);
    }  catch (std::exception& e) {
        std::cerr << e.what();
    }

    return 0;
}

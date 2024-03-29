#ifndef EVENT_WRAPPERS_HPP
#define EVENT_WRAPPERS_HPP

#include <event2/event.h>
#include <event2/thread.h>
#include <event2/bufferevent.h>

#include <stdexcept>
#include <string_view>

namespace libevent {

struct event_base_w
{
    event_base* base;

    event_base_w()
    {
        base = event_base_new();
        if(!base)
            throw std::runtime_error("Cannot create new event_base\n");
    }
    ~event_base_w()
        { event_base_free(base); }
    event_base_w(const event_base_w& other) = delete;
    event_base_w& operator=(const event_base_w& other) = delete;
    event_base_w(event_base_w&& other)
        : base(other.base)
    { other.base = nullptr; }
    event_base_w& operator=(event_base_w&& other)
    {
        event_base_free(base);
        base = other.base;
        other.base = nullptr;

        return *this;
    }
};

struct bufferevent_w
{
    std::size_t bytes_read_threshold;
    std::size_t bytes_send_threshold;
    std::size_t bytes_read;
    std::size_t bytes_send;
    bufferevent* bev;
    std::string_view send_message;

    bufferevent_w(event_base_w& ebw, std::size_t brthsh, std::size_t bsthsh, const std::string_view& message)
        : bytes_read_threshold(brthsh),
        bytes_send_threshold(bsthsh),
        bytes_read(0),
        bytes_send(0),
        send_message(message)
    {
         bev = bufferevent_socket_new(ebw.base, -1, BEV_OPT_CLOSE_ON_FREE);
         if(!bev)
             throw std::runtime_error("Cannot create bufferevent\n");
    }

    bufferevent_w(bufferevent_w&& other)
        : bytes_read_threshold(other.bytes_read_threshold),
        bytes_send_threshold(other.bytes_send_threshold),
        bytes_read(other.bytes_read),
        bytes_send(other.bytes_send),
        bev(other.bev),
        send_message(other.send_message)
        { other.bev = nullptr; }

    bufferevent_w& operator=(bufferevent_w&& other)
    {
        bufferevent_free(bev);
        bev = other.bev;
        other.bev = nullptr;
        bytes_read = other.bytes_read;
        bytes_read = other.bytes_send;
        bytes_read_threshold = other.bytes_read_threshold;
        send_message = other.send_message;

        return *this;
    }
    bufferevent_w(const bufferevent_w& other) = delete;
    bufferevent_w& operator=(const bufferevent_w& other) = delete;

    void close()
    {
        if(bev != nullptr){
            bufferevent_free(bev);
            bev = nullptr;
        }
    }

    ~bufferevent_w()
        {
            if(bev != nullptr)
                bufferevent_free(bev);
        }
};

struct event_w
{
    struct event* ev;
    event_w() : ev(nullptr)
    {}

    event_w(event_w&& other)
        : ev(other.ev)
    { other.ev = nullptr;}

    event_w& operator=(event_w&& other)
    {
        event_free(ev);
        ev = other.ev;
        other.ev = nullptr;

        return *this;
    }
    event_w(const event_w& other) = delete;
    event_w& operator=(const event_w& other) = delete;

    ~event_w()
        { event_free(ev); }
};


}


#endif // EVENT_WRAPPERS_HPP

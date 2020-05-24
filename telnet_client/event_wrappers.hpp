#ifndef EVENT_WRAPPERS_HPP
#define EVENT_WRAPPERS_HPP

#include <event2/event.h>
#include <event2/thread.h>
#include <event2/bufferevent.h>

#include <stdexcept>

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
    }
};

struct bufferevent_w
{
    bufferevent* bev;

    bufferevent_w(event_base_w& ebw)
    {
         bufferevent_socket_new(ebw.base, -1, BEV_OPT_CLOSE_ON_FREE);
         if(!bev)
             throw std::runtime_error("Cannot create bufferevent\n");
    }
    bufferevent_w(bufferevent_w&& other)
        : bev(other.bev)
        { other.bev = nullptr; }
    bufferevent_w& operator=(bufferevent_w&& other)
    {
        bufferevent_free(bev);
        bev = other.bev;
        other.bev = nullptr;
    }
    bufferevent_w(const bufferevent_w& other) = delete;
    bufferevent_w& operator=(const bufferevent_w& other) = delete;

    ~bufferevent_w()
        { bufferevent_free(bev); }
};

}


#endif // EVENT_WRAPPERS_HPP

#pragma once

#include "util.h"
#include "connection.hpp"

#ifdef SYSTEM_WIN_64
enum Poll_Event {
    EVENT_READ  = POLLRDNORM,
    EVENT_WRITE = POLLWRNORM,
    EVENT_ERROR = POLLERR | 
                  POLLHUP | 
                  POLLNVAL
};
#elif defined(SYSTEM_UNX)
#   include <poll.h>
enum Poll_Event {
    EVENT_READ  = POLLIN,
    EVENT_WRITE = POLLOUT,
    EVENT_ERROR = POLLERR | 
                  POLLHUP | 
                  POLLNVAL
};
#endif

template<typename T>
struct Client_Poll {
    Client_Poll(Connection<T>& conn) : conn(conn) {}

    inline void set_socket(socket_t sock)
    {
        poll_fds.fd = sock;
    }

    inline auto& get_events()
    {
        return poll_fds.events;
    }

    inline void set_events(i32 event)
    {
        poll_fds.events = event;
    }

    i32 poll(u32 timeout_usec)
    {
#ifdef SYSTEM_UNX
        return ::poll(&poll_fds, 1, timeout_usec);
#elif defined(SYSTEM_WIN_64)
        return WSAPoll(&poll_fds, 1, timeout_usec);
#endif
    }

    inline bool is_set(Poll_Event event)
    {
        return poll_fds.revents & event;
    }

private:
#ifdef SYSTEM_WIN_64
    WSAPOLLFD poll_fds;
#elif defined(SYSTEM_UNX)
    struct pollfd poll_fds;
#endif
    Connection<T>& conn;
};

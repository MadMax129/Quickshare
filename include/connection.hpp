#pragma once

#include "util.h"
#include <optional>
#include <utility>

#ifdef SYSTEM_WIN_64
#   include <WS2tcpip.h>
#   include <winsock2.h>
#   include <windows.h>
#   include <iphlpapi.h>
#   define CLOSE_SOCKET(sock) ::closesocket(sock)
using socket_t = SOCKET;
#elif defined(SYSTEM_UNX)
#   include <sys/socket.h>
#   include <unistd.h>
#   include <arpa/inet.h>
#   define CLOSE_SOCKET(sock) ::close(sock)
using socket_t = int;
#endif

using Sock_Info = std::optional<std::pair<socket_t, sockaddr_in>>;

template <typename T>
struct Connection {
    Connection() : my_sock(0) {}

    bool create_socket(const char* ip, u16 port)
    {
        my_sock = socket(AF_INET, SOCK_STREAM, 0);

#ifdef SYSTEM_WIN_64
        if (my_sock == INVALID_SOCKET)
            return false;
#elif defined(SYSTEM_UNX)
        if (my_sock < 0)
            return false;
#endif
        my_addr.sin_family = AF_INET;
        my_addr.sin_addr.s_addr = ip ? inet_addr(ip) : INADDR_ANY;
        my_addr.sin_port = htons(port);

        return true;
    }

    bool bind_and_listen() const
    {
        if (bind(my_sock, (struct sockaddr*)&my_addr, sizeof(my_addr)) < 0)
            return false;

        if (listen(my_sock, SOMAXCONN) < 0)
            return false;

        return true;
    }

    bool recv(const socket_t sock, T* buf) const
    {
        ssize_t to_read = sizeof(T);
        char* buf_ptr = reinterpret_cast<char*>(buf);

        while (to_read > 0) {
            const ssize_t r_bytes = ::recv(
                sock, 
                buf_ptr,
                to_read,
                0
            );

            if (r_bytes <= 0) // ? could interupt happen during this loop??
                return false;

            to_read -= r_bytes;
            buf_ptr += r_bytes;
        }

        return true;
    }

    bool send(const socket_t sock, const T* buf) const
    {
        ssize_t to_send = sizeof(T);
        const char* buf_ptr = reinterpret_cast<const char*>(buf);

        while (to_send > 0) {
            const ssize_t s_bytes = ::send(
                sock, 
                buf_ptr, 
                to_send, 
                0
            );

            if (s_bytes <= 0)
                return false;

            to_send -= s_bytes;
            buf_ptr += s_bytes;
        }

        return true;
    }

    bool send_and_recv(const socket_t sock, T* buf) const 
    {
        if (!send(sock, buf))
            return false;
        
        if (!recv(sock, buf))
            return false;

        return true;
    }

    bool connect() const 
    {
        if (::connect(my_sock,
                    reinterpret_cast<const sockaddr*>(&my_addr),
                    sizeof(my_addr)) < 0)
            return false;
        
        return true;
    }

    Sock_Info accept() const
    {
        std::pair<socket_t, sockaddr_in> info;
        socklen_t len = sizeof(sockaddr_in);

        info.first = ::accept(
            my_sock, 
            reinterpret_cast<sockaddr*>(&info.second), 
            &len
        );

#ifdef SYSTEM_WIN_64
        if (info.first == INVALID_SOCKET)
#elif defined(SYSTEM_UNX)
        if (info.first < 0)
#endif
            return {};

        return info;
    }

    void close()
    {
        CLOSE_SOCKET(my_sock);
        my_sock = 0;
    }

    void set_sock_timeout(const socket_t sock, const u32 sec)
    {
#ifdef SYSTEM_WIN_64
        DWORD timeout = sec * 1000;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
#elif defined(SYSTEM_UNX)
        struct timeval timeout;
        timeout.tv_sec = sec;
        timeout.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
#else
#   error "Unsupported target"
#endif
    }

    inline socket_t me() const 
    { 
        return my_sock; 
    }

    inline const sockaddr_in& get_addr() const 
    { 
        return my_addr; 
    }

private:
    socket_t my_sock;
    sockaddr_in my_addr;
};
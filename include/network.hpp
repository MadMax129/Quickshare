#pragma once

#ifdef SYSTEM_WIN_64
#   include <WS2tcpip.h>
#elif defined(SYSTEM_UNX)
#   include<sys/types.h>
#   include<sys/socket.h>
#   include<sys/un.h>
#   include<arpa/inet.h>
#endif
#include "quickshare.hpp"
#include <thread>
#include <atomic>
#include <optional>

#define PACKET_MAX_SIZE 512
#define STATIC_SERVER_PORT 8345

struct Network {
public: 
    Network() = default;
    ~Network();

    bool init_socket();
    bool send_intro(char* username);
    void start_connection();

    enum {
        INACTIVE,
        FAILED_CONNECTION,
        IS_SERVER,
        IS_CLIENT,
    };

private:
    void try_connect();
#ifdef SYSTEM_WIN_64
    WSAData _wsa_data;
    using socket_t = SOCKET;
#elif defined(SYSTEM_UNX)
    using socket_t = int;
#endif
    socket_t tcp_socket;
    sockaddr_in server_addr;
    std::atomic<u32> state{INACTIVE};
    std::thread recv_th;
};

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
#include <thread>

/* Defines the size of a packet send through the network. */
#define PACKET_MAX_SIZE 512

/* Arbitrary server port that defines the central server location on local network */
#define STATIC_SERVER_PORT 8345

/* Defines the max number of clients accepted */
#define MAX_CLIENTS 32

#define CLIENT_NAME_LEN 30

#ifdef SYSTEM_WIN_64
    typedef SOCKET socket_t;
#elif defined(SYSTEM_UNX)
    typedef int socket_t;
#endif

struct Msg {
    enum : u8 {
        INVALID,
        NAME_SEND,
    } type;
};

struct Client {
    enum {
        EMPTY,
        OPEN,
        COMPLETE
    } state;
    u32 id;
    wchar_t name[CLIENT_NAME_LEN];
    struct sockaddr_in addr;
    socket_t socket;
};

struct Network {
public: 
    Network() = default;
    ~Network();

    bool init_socket();
    void network_loop();

    enum {
        INACTIVE,
        FAILED_CONNECTION,
        IS_SERVER,
        IS_CLIENT,
    };

private:
    void try_connect();
    void cli_loop();
    void server_loop();
    Client* new_client();
    void construct_client(socket_t socket, struct sockaddr_in* addr);
    void debug_clients();

    Client client_list[MAX_CLIENTS] = {};
    u32 client_count{0};
#ifdef SYSTEM_WIN_64
    WSAData wsa_data;
#endif
    u32 cli_id{0};
    socket_t tcp_socket;
    sockaddr_in server_addr;
    std::atomic<u32> state{INACTIVE};
    std::thread conn_thread;
};

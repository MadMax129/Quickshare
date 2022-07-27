#pragma once

// #ifdef SYSTEM_WIN_64
#   include <WS2tcpip.h>
#   include <winsock2.h>
// #elif defined(SYSTEM_UNX)
// #   include<sys/types.h>
// #   include<sys/socket.h>
// #   include<sys/un.h>
// #   include<arpa/inet.h>
// #endif
#include "quickshare.hpp"
#include <thread>
#include <atomic>
#include <optional>
#include <thread>
#include <array>
#include <cstring>

/* Defines the size of a packet send through the network. */
#define PACKET_MAX_SIZE 1300

/* Arbitrary server port that defines the central server location on local network */
#define STATIC_SERVER_PORT 8345

/* Defines the max number of clients accepted */
#define MAX_CLIENTS 32

#define CLIENT_NAME_LEN 16
#define MAX_FILE_NAME 64

#ifdef SYSTEM_WIN_64
    typedef SOCKET socket_t;
#elif defined(SYSTEM_UNX)
    typedef int socket_t;
#endif

struct Msg {
    enum Msg_Type : u8 {
        /* Empty message */
        INVALID,
        /* Completes the client initialization */
        NAME_SEND,
        /* Echos client list and id's */
        CLIENT_LIST,
        /* Request sending a file / folder */
        REQUEST
    };

    struct {
        Msg_Type type;
        u8 recipient_id;
        u16 : 16;
    } hdr;

    union {
        struct {
            struct {
                u8 id;
                wchar_t name[CLIENT_NAME_LEN];
            } clients[MAX_CLIENTS];
            u8 client_count;
        } list;
        struct {
            u32 file_size;
            u32 packets;
            char file_name[MAX_FILE_NAME];
        } request;
        u8 buffer[PACKET_MAX_SIZE - sizeof(hdr)];
    };
};

static_assert(sizeof(Msg) == PACKET_MAX_SIZE);

struct Client {
    Client() = default;
    Client(struct sockaddr_in* addr, 
           socket_t sock,
           u8 id) {
        this->id = id;
        this->addr = *addr;
        this->socket = sock;
        this->state = OPEN;
    }
    enum {
        EMPTY,
        OPEN,
        COMPLETE
    } state;
    u8 id;
    wchar_t name[CLIENT_NAME_LEN];
    struct sockaddr_in addr;
    socket_t socket;
};

struct Network {
public: 
    Network() = default;
    ~Network();

    /* Initialize network socket and return sucess */
    bool init_socket();

    /* Loop function interface */
    void network_loop();

    enum {
        INACTIVE,
        FAILED_CONNECTION,
        IS_SERVER,
        IS_CLIENT,
    };

private:
    /* Clean up all network resources */
    void cleanup();
    
    /* Adds a new client into the client_list and returns pointer to element */
    Client* new_client();

    /* Removes client from client_list and adjusts client_count */
    void remove_client(socket_t sock);

    u8 get_id() const;

    /* Attempts to connect to server, exits only if connected */
    void try_connect();
    
    /* Main threads for client or server networks */
    void cli_loop();
    void server_loop();
    
    /* Analize message send from clients */
    void server_analize_msg(const Msg& msg, socket_t socket);
    
    void debug_clients();

    std::array<Client, MAX_CLIENTS> client_list{};
    u32 client_count{0};
#ifdef SYSTEM_WIN_64
    WSAData wsa_data;
#endif
    u8 cli_id{0};
    fd_set master_fds, work_fds;
    socket_t tcp_socket;
    sockaddr_in server_addr;
    std::atomic<u32> state{INACTIVE};
    std::thread conn_thread;
};

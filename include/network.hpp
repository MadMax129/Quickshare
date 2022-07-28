#pragma once

#include <WS2tcpip.h>
#include <winsock2.h>
#include "quickshare.hpp"
#include <thread>
#include <atomic>
#include <optional>
#include <thread>
#include <array>
#include <cstring>
#include <ctime>

/* Defines the size of a packet send through the network. */
#define PACKET_MAX_SIZE 1304

/* Arbitrary server port that defines the central server location on local network */
#define STATIC_SERVER_PORT 8345

/* Defines the max number of clients accepted */
#define MAX_CLIENTS 16

/* Max length for display computer host name */
#define CLIENT_NAME_LEN 16

/* Max file name */
#define MAX_FILE_NAME 64

#ifdef SYSTEM_WIN_64
    typedef SOCKET socket_t;
#elif defined(SYSTEM_UNX)
    typedef int socket_t;
#endif

typedef time_t UserId;

struct File_Manager;

struct Request {
    u32 file_size;
    u32 packets;
    char file_name[MAX_FILE_NAME];
};

struct Msg {
    enum Msg_Type : u8 {
        /* Empty message */
        INVALID,
        /* Server/client response for request */
        REJECTED, 
        /* Server/client response for request */
        ACCEPTED, 
        /* Packet */
        PACKET,
        /* Completes the client initialization */
        NAME_SEND,
        /* Echos client list and id's */
        CLIENT_LIST,
        /* Request sending a file / folder */
        REQUEST
    };

    struct {
        UserId recipient_id;
        UserId sender_id;
        Msg_Type type;
    } hdr;

    union {
        struct {
            struct {
                UserId id;
                wchar_t name[CLIENT_NAME_LEN];
            } clients[MAX_CLIENTS];
            u8 client_count;
        } list;
        struct Request request;
        struct {
            u16 packet_size; // (1, PACKET_MAX_SIZE)
            u8 bytes[PACKET_MAX_SIZE - sizeof(hdr) - sizeof(u16)];
        } packet;
        u8 buffer[PACKET_MAX_SIZE - sizeof(hdr)];
    };
};

static_assert(sizeof(Msg) == PACKET_MAX_SIZE);

struct Client {
    enum {
        EMPTY,
        OPEN,
        COMPLETE
    } state;
    UserId id;
    wchar_t name[CLIENT_NAME_LEN];
    struct sockaddr_in addr;
    socket_t socket;
};

struct Network {
public: 
    Network(File_Manager* f);
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
    Client* new_client(struct sockaddr_in* addr, socket_t sock);

    /* Removes client from client_list and adjusts client_count */
    void remove_client(socket_t sock);

    inline Client* get_client(socket_t sock);

    UserId get_id() const;

    /* Attempts to connect to server, exits only if connected */
    void try_connect();
    
    /* Main threads for client or server networks */
    void cli_loop();
    void server_loop();
    
    /* Analize message send from clients */
    void server_analize_msg(const Msg& msg, socket_t socket);
    void client_list_msg(const Msg& msg, Client* cli);
    void send_client_list(Client* cli);
    void analize_request(const Msg& msg, Client* cli);
    void analize_packet(const Msg& msg, Client* cli);

    /* Client message handlers */
    void handle_request(const Msg& msg, Client* cli);
    void handle_packet(const Msg& msg, Client* cli);
    
    void debug_clients() const;

    std::array<Client, MAX_CLIENTS> client_list{};
    u32 client_count{0};
#ifdef SYSTEM_WIN_64
    WSAData wsa_data;
#endif
    Client* self;
    File_Manager* const f_manager;
    fd_set master_fds, work_fds;
    socket_t tcp_socket;
    Msg* temp_msg_buf;
    sockaddr_in server_addr;
    std::atomic<u32> state{INACTIVE};
    std::thread conn_thread;
};

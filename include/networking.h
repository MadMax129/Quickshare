#pragma once

#include <WS2tcpip.h>
#include <thread>
#include <mutex>
#include <atomic>

#define USERNAME_MAX_LIMIT 16
#define PACKET_MAX_SIZE 512

enum class Msg_Type : unsigned char {
    SERROR,         // Server Error
    NEW_CLIENT,     // Client connecting to server
    CLIENT_ID,      // Server sends ID to client
    
    GLOBAL_CHAT,    // Client or Server chat message
    
    USER_ADD,       // Add client to active list
    USER_REMOVE,    // Remove client from active list
};

struct Tcp_Msg {
    struct Id {
        unsigned char username[USERNAME_MAX_LIMIT];
    };
    struct Chat_Msg {
        unsigned char username[USERNAME_MAX_LIMIT];
        unsigned char data[PACKET_MAX_SIZE    - 
                           USERNAME_MAX_LIMIT - 
                           sizeof(Msg_Type)];
    };
    union {
        Id id;
        Chat_Msg msg;
    };
    Msg_Type m_type;
};

static_assert(sizeof(Msg_Type) == sizeof(char));
static_assert(sizeof(Tcp_Msg) == PACKET_MAX_SIZE);

#define MAX_QUEUE_SIZE 100

struct Msg_Queue {
public:
    Msg_Queue();
    ~Msg_Queue();

    void pop(Tcp_Msg* buf);
    Msg_Type peek();
    void push(Tcp_Msg* item);
    int get_size();
    
private:
    int front, back, size;
    Tcp_Msg* queue;
    std::mutex mutex;
};

struct Client_Sock {
public: 
    explicit Client_Sock(const char* ip, const unsigned short port);
    ~Client_Sock();

    bool init_socket();
    bool send_intro(char* username);
    void start_recv();
    void start_connection();
    int has_connected();
    void disconnect();

    Msg_Queue msg_queue;
    std::atomic<int> connected;

private:
    unsigned short _port;
    const char *_ip;
    WSAData _wsa_data;
    SOCKET _tcp_socket;
    SOCKET _udp_socket;
    sockaddr_in _server_addr;
    std::thread server_read;

    void try_connect();
    void recv_thread();
};

void test();
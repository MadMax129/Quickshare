#pragma once

#include <WS2tcpip.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include "message.h"

#define MAX_QUEUE_SIZE 16

struct Msg_Queue {
public:
    Msg_Queue();
    ~Msg_Queue();

    Tcp_Msg* pop();
    Tcp_Msg* peek();
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
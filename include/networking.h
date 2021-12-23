#pragma once

#include <WS2tcpip.h>
#include <thread>
#include <queue>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include "message.h"

struct MsgQueue {
public:
    MsgQueue();
    ~MsgQueue();

    Tcp_Msg* pop();
    void push(Tcp_Msg item);
    unsigned int size();
    
private:
    std::queue<Tcp_Msg> _queue;
    std::mutex _mutex;
};

struct ClientSock {
public: 
    explicit ClientSock(const char* ip, const unsigned short port);
    ~ClientSock();

    bool init_socket();
    bool send_intro(char* username);
    void start_recv();
    void start_connection();
    int has_connected();

    MsgQueue msg_queue;

private:
    unsigned short _port;
    const char *_ip;
    WSAData _wsa_data;
    SOCKET _tcp_socket;
    SOCKET _udp_socket;
    sockaddr_in _server_addr;
    std::thread server_read;
    std::atomic<int> _connected;

    void try_connect();
    void recv_thread();
};

void test();
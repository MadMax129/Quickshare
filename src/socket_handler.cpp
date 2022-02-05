#include "networking.h"
#include <windows.h>
#include "quickshare.h"
#include <assert.h>

Client_Sock::Client_Sock(const char *ip, const unsigned short port) :
    _port(port), _ip(ip)
{
    connected.store(State::INACTIVE);
}

Client_Sock::~Client_Sock() 
{
    disconnect();
}

void Client_Sock::disconnect()
{
    WSACleanup();
    closesocket(_tcp_socket);
}

void Client_Sock::recv_thread()
{
    static Tcp_Msg global_msg = {0};

    // Try to connect first
    try_connect();

    for (;;)
    {
        int result = recv(_tcp_socket, (char*)&global_msg, sizeof(global_msg), 0);
        
        switch (result)
        {
            // Server gracefully shut off, must disconnect first
            case 0: {
                LOGGER("Server gracefully shut off...\n");
                disconnect();
                init_socket();
                connected.store(State::FAILED);
                break;
            }
            
            // Server returned error 
            case SOCKET_ERROR: {
                LOGGER("Server connecting issue...\n");

                do {
                    Sleep(2000);
                    try_connect();
                } while (connected.load() != State::CONNECTED);

                break;
            }

            // Got message from server
            default: {
                msg_queue.push(&global_msg);
                break;
            }
        }
    }
}

Client_Sock::State Client_Sock::get_state() const
{
    return static_cast<State>(connected.load());
}

bool Client_Sock::init_socket() 
{
    if (WSAStartup(MAKEWORD(2,2), &_wsa_data) != 0)
        return false;

        // all this needs to be restarted when connection fails

    _tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    // Eventually add udp socket

    if (_tcp_socket == INVALID_SOCKET)
        return false;

    _server_addr.sin_family = AF_INET;
	_server_addr.sin_addr.s_addr = inet_addr(_ip);
	_server_addr.sin_port = htons(_port);

    return true;
}

void Client_Sock::try_connect()
{
    LOGGER("Attempting to connect to server...\n");
    if (connect(_tcp_socket, (struct sockaddr*)&_server_addr,
                            sizeof(_server_addr)) == SOCKET_ERROR) {
        connected.store(State::FAILED);
    }
    else {
        LOGGER("Connected to server!\n");
        connected.store(State::CONNECTED);
    }
}

void Client_Sock::start_connection()
{
    recv_th = std::thread(&Client_Sock::recv_thread, this);
    recv_th.detach();
}

bool Client_Sock::send_intro(char* username)
{
    static struct Tcp_Msg global_msg;
    memset(&global_msg, 0, sizeof(global_msg));
    global_msg.m_type = Msg_Type::NEW_CLIENT;
    memcpy(global_msg.id.username, username, USERNAME_MAX_LIMIT);
    if (send(_tcp_socket, (const char*)&global_msg, sizeof(global_msg), 0) == SOCKET_ERROR)
        return false;

    return true;
}

Msg_Queue::Msg_Queue() 
{
    front = 0;
    back = -1;
    size = 0;
    queue = (Tcp_Msg*)malloc(sizeof(Tcp_Msg) * MAX_QUEUE_SIZE);
    if (!queue)
        FATAL_MEM();
    memset(queue, 0, sizeof(Tcp_Msg[MAX_QUEUE_SIZE]));
}

Msg_Queue::~Msg_Queue() 
{
    free(queue);
}

void Msg_Queue::pop(Tcp_Msg* buf)
{
    mutex.lock();
    assert(size >= 0);

    memcpy(buf, &queue[front++], sizeof(Tcp_Msg));

    if (front == MAX_QUEUE_SIZE)
        front = 0;

    size--;
    mutex.unlock();
}

Msg_Type Msg_Queue::peek()
{
    mutex.lock();
    Msg_Type copy = queue[front].m_type;
    mutex.unlock();
    return copy;
}

void Msg_Queue::push(Tcp_Msg* item)
{
    mutex.lock();
    assert(size != MAX_QUEUE_SIZE);

    if (back == MAX_QUEUE_SIZE-1)
        back = -1;

    back++;
    memcpy(&queue[back], item, sizeof(Tcp_Msg));
    size++;
    mutex.unlock();
}

int Msg_Queue::get_size() 
{
    mutex.lock();
    int s = size;
    mutex.unlock();
    return s;
}

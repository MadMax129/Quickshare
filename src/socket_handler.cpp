#include "networking.h"
#include <windows.h>
#include "quickshare.h"
#include <assert.h>

static Tcp_Msg global_msg;

Client_Sock::Client_Sock(const char *ip, const unsigned short port) :
    _port(port), _ip(ip) {}

Client_Sock::~Client_Sock() 
{
    disconnect();
}

void Client_Sock::disconnect()
{
    WSACleanup();
    closesocket(_tcp_socket);
    // closesocket(_udp_socket);
}

void Client_Sock::recv_thread()
{
    for (;;)
    {
        int result = recv(_tcp_socket, (char*)&global_msg, sizeof(global_msg), 0);
        
        switch (result)
        {
            case 0: case SOCKET_ERROR: {
                LOGGER("Server recv error\n");
                connected = 0;
                disconnect();
                return;
            }

            default: {
                msg_queue.push(&global_msg);
                break;
            }
        }
    }
}

void Client_Sock::start_recv()
{
    server_read = std::thread(&Client_Sock::recv_thread, this);
    server_read.detach();
}

bool Client_Sock::init_socket() 
{
    if (WSAStartup(MAKEWORD(2,2), &_wsa_data) != 0)
        return false;

    _tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    // _udp_socket = socket(AF_INET, SOCK_DGRAM, 0);

    // if (_tcp_socket == INVALID_SOCKET || _udp_socket == INVALID_SOCKET)
        // return false;

    if (_tcp_socket == INVALID_SOCKET)
        return false;

    _server_addr.sin_family = AF_INET;
	_server_addr.sin_addr.s_addr = inet_addr(_ip);
	_server_addr.sin_port = htons(_port);

    return true;
}

void Client_Sock::try_connect()
{
    if (connect(_tcp_socket, (struct sockaddr*)&_server_addr,
                            sizeof(_server_addr)) == SOCKET_ERROR)
        connected = 0;
    else
        connected = 1;
}

int Client_Sock::has_connected()
{
    int copy = connected;
    return copy;
}

void Client_Sock::start_connection()
{
    connected = -1;
    std::thread conn(Client_Sock::try_connect, this);
    conn.detach();
}

bool Client_Sock::send_intro(char* username)
{
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

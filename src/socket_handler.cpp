#include "networking.h"
#include <windows.h>
#include "quickshare.h"

static Tcp_Msg global_msg;

ClientSock::ClientSock(const char *ip, const unsigned short port) :
    _port(port), _ip(ip) {}

ClientSock::~ClientSock() 
{
    WSACleanup();
    closesocket(_tcp_socket);
    closesocket(_udp_socket);
    server_read.join();
}

void ClientSock::recv_thread()
{
    memset(&global_msg, 0, sizeof(global_msg));
    for (;;)
    {
        int result = recv(_tcp_socket, (char*)&global_msg, sizeof(global_msg), 0);
        
        switch (result)
        {
            case 0:
                LOGGER("Server disconneted...\n");
                exit(1);
                break;
            
            case SOCKET_ERROR:
                return;

            default:
                msg_queue.push(global_msg);
                break;
        }
    }
}

void ClientSock::start_recv()
{
    server_read = std::thread(&ClientSock::recv_thread, this);
}

bool ClientSock::init_socket() 
{
    if (WSAStartup(MAKEWORD(2,2), &_wsa_data) != 0)
        return false;

    _tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    _udp_socket = socket(AF_INET, SOCK_DGRAM, 0);

    if (_tcp_socket == INVALID_SOCKET || _udp_socket == INVALID_SOCKET)
        return false;

    _server_addr.sin_family = AF_INET;
	_server_addr.sin_addr.s_addr = inet_addr(_ip);
	_server_addr.sin_port = htons(_port);

    return true;
}

void ClientSock::try_connect()
{
    if (connect(_tcp_socket, (struct sockaddr*)&_server_addr,
                            sizeof(_server_addr)) == SOCKET_ERROR)
        _connected = 0;
    else
        _connected = 1;
}

int ClientSock::has_connected()
{
    int copy = _connected;
    return copy;
}

void ClientSock::start_connection()
{
    _connected = -1;
    std::thread conn(ClientSock::try_connect, this);
    conn.detach();
}

bool ClientSock::send_intro(char* username)
{
    memset(&global_msg, 0, sizeof(global_msg));
    global_msg.m_type = M_NEW_CLIENT;
    memcpy(global_msg.data.intro.username, username, USERNAME_MAX_LIMIT);
    if (send(_tcp_socket, (const char*)&global_msg, sizeof(global_msg), 0) == SOCKET_ERROR)
        return false;

    return true;
}

MsgQueue::MsgQueue() {}
MsgQueue::~MsgQueue() {}

Tcp_Msg* MsgQueue::pop()
{
    _mutex.lock();
    Tcp_Msg* result = new Tcp_Msg;
    memset(result, 0, sizeof(Tcp_Msg));
    memcpy(result, &_queue.front(), sizeof(Tcp_Msg));
    _queue.pop();
    _mutex.unlock();
    return result;
}

void MsgQueue::push(Tcp_Msg item)
{
    _mutex.lock();
    _queue.push(item);
    _mutex.unlock();
}

unsigned int MsgQueue::size() 
{
    _mutex.lock();
    unsigned int size = _queue.size();
    _mutex.unlock();

    return size;
}
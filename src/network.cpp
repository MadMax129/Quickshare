#include "network.hpp"
#include "quickshare.hpp"
#include <assert.h>

Network::~Network() 
{
#ifdef SYSTEM_WIN_64
    closesocket(tcp_socket);
    WSACleanup();
#elif defined(SYSTEM_UNX)
    close(tcp_socket)l
#endif
}

bool Network::init_socket()
{
#ifdef SYSTEM_WIN_64
    if (WSAStartup(MAKEWORD(2,2), &_wsa_data) != 0)
        return false;
#endif
    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (tcp_socket < 0) 
        return false;

    server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_port = htons(STATIC_SERVER_PORT);

    if (bind(tcp_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        LOG("Attempting to connect to server...\n");
        // try_connect(); must be done async
    }
    else {
        if (listen(tcp_socket, SOMAXCONN) < 0)
            return false;
    }

    return true;
}

void Network::try_connect()
{
    do {
        if (connect(tcp_socket, 
                    (struct sockaddr*)&server_addr, 
                    sizeof(server_addr)) < 0) {
            state.store(FAILED_CONNECTION); 
            Sleep(2000);
        }
        else {
            LOG("Connected to server!\n");
            state.store(IS_CLIENT);
        }
    } while (state.load() == FAILED_CONNECTION);
}

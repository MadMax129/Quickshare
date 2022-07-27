#include "network.hpp"
#include "quickshare.hpp"
#include <assert.h>

static void get_comp_name(wchar_t buffer[CLIENT_NAME_LEN])
{
    DWORD buf_size = CLIENT_NAME_LEN;
    GetComputerName((LPSTR)buffer, &buf_size);
}

Network::~Network() 
{
#ifdef SYSTEM_WIN_64
    closesocket(tcp_socket);
    WSACleanup();
#elif defined(SYSTEM_UNX)
    close(tcp_socket);
#endif
}

Client* Network::new_client()
{
    i32 idx = -1;
    for (i32 i = 0; i < MAX_CLIENTS; i++) {
        if (client_list[i].state == Client::EMPTY) {
            client_list[i].state = Client::OPEN;
            idx = i;
            break;
        }
    }
    assert(idx != -1);
    return &client_list[idx];
}

void Network::debug_clients()
{
    for (i32 i = 0; i < MAX_CLIENTS; i++) {
        const Client& cli = client_list[i];
        if (cli.state == Client::COMPLETE) {
            colored_printf(CL_BLUE, "%s:\n", cli.name);
            printf("\t'%s:%d'\n", inet_ntoa(cli.addr.sin_addr), cli.addr.sin_port);
        }
    }
}

bool Network::init_socket()
{
#ifdef SYSTEM_WIN_64
    if (WSAStartup(MAKEWORD(2,2), &wsa_data) != 0)
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
        try_connect(); // Wait till connection
    }
    else {
        LOG("Created server...\n");
        if (listen(tcp_socket, SOMAXCONN) < 0)
            return false;
        state.store(IS_SERVER);
        
        Client* host_client = new_client();
        assert(host_client);
        get_comp_name(host_client->name);
        host_client->addr = server_addr;
        host_client->socket = tcp_socket;
        host_client->id = cli_id++;
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

void Network::network_loop()
{
    u32 s = state.load(std::memory_order_relaxed);

    if (s == IS_SERVER)
        conn_thread = std::thread(&Network::server_loop, this);
    else if (s == IS_CLIENT)
        conn_thread = std::thread(&Network::cli_loop, this);

    conn_thread.join();
}

void Network::cli_loop()
{
    Msg msg = {};
    msg.type = Msg::NAME_SEND;
    assert(send(tcp_socket, (char*)&msg, sizeof(Msg), 0) != SOCKET_ERROR);
}

void Network::construct_client(socket_t socket, struct sockaddr_in* addr)
{
    Client* c = new_client();
    c->addr = *addr;
    c->socket = socket;
    c->id = cli_id++;
}

void Network::server_loop()
{
    fd_set master_fds, work_fds;
    
    FD_ZERO(&master_fds);
    FD_SET(tcp_socket, &master_fds);

    for (;;) {
        work_fds = master_fds;
        
        const i32 res = select(0, &work_fds, NULL, NULL, NULL);

        if (res < 0) {
            LOGF("ERROR SELCET %d\n", WSAGetLastError());
            exit(1);
        }

        LOGF("%d\n", res);

        for (i32 i = 0; i < res; i++) {
            socket_t sock = work_fds.fd_array[i];

            if (sock == tcp_socket) {
                struct sockaddr_in addr;
                int len = sizeof(addr);

                socket_t client = accept(tcp_socket, (struct sockaddr*)&addr, &len);

                if (client_count > MAX_CLIENTS) {
                    LOGF("Client rejected '%s:%d'\n", inet_ntoa(addr.sin_addr), addr.sin_port);
                    closesocket(client);
                }
                else {
                    LOGF("Client connected '%s:%d'\n", inet_ntoa(addr.sin_addr), addr.sin_port);
                    FD_SET(client, &master_fds);
                    construct_client(client, &addr);
                    client_count++;
                }
            }
            else {
                Msg buf = {};

                i32 recv_bytes = recv(sock, (char*)&buf, sizeof(Msg), 0);
                if (recv_bytes <= 0) {
                    LOG("Disconnect\n");
                    closesocket(sock);
                    FD_CLR(sock, &master_fds);
                }
                else 
                    printf("==>%d\n", buf.type);

            }
        }
        
    }
}

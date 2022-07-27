#include "network.hpp"
#include "quickshare.hpp"
#include <assert.h>
#include <algorithm>
#include <iterator>

static void get_comp_name(TCHAR buffer[CLIENT_NAME_LEN])
{
    DWORD buf_size = CLIENT_NAME_LEN;
    GetComputerName(buffer, &buf_size);
}

Network::~Network() 
{
    cleanup();
}

void Network::cleanup()
{
    FD_CLR(tcp_socket, &master_fds);
#ifdef SYSTEM_WIN_64
    closesocket(tcp_socket);
    WSACleanup();
#elif defined(SYSTEM_UNX)
    close(tcp_socket);
#endif
    while (master_fds.fd_count > 0) {
        socket_t sock = master_fds.fd_array[0];
        FD_CLR(sock, &master_fds);
        closesocket(sock);
    }
    client_list.fill({});
    client_count = 0;
    cli_id = 0;
}

Client* Network::new_client()
{
    auto cli = std::find_if(
            std::begin(client_list), 
            std::end(client_list),
            [] (const Client &c) { return c.state == Client::EMPTY; }
    );

    cli->id = get_id();

    ++client_count;

    assert(cli != std::end(client_list));

    return cli;
}

u8 Network::get_id() const 
{
    u8 lowest = 0;
    bool looping = true;

    if (client_count == 0)
        return 0;

    while (looping) {
        for (u8 i = 0; i < MAX_CLIENTS; i++) {
            if (client_list[i].state != Client::EMPTY && 
                client_list[i].id == lowest) {
                ++lowest;
                looping = false;
                break;
            }
        }
    }

    return lowest;
}

void Network::remove_client(socket_t sock)
{
    auto cli = std::find_if(
        std::begin(client_list),
        std::end(client_list),
        [&] (const Client& c) { return c.socket == sock; }
    );

    assert(cli != std::end(client_list));
    
    closesocket(sock);
    FD_CLR(sock, &master_fds);
    std::memset(cli, 0, sizeof(Client));
    --client_count;
}

void Network::debug_clients()
{
    printf("Client amount: %u\n", client_count);
    for (const Client& cli : client_list) {
        if (cli.state != Client::EMPTY) {
            if (cli.state == Client::OPEN) {
                colored_printf(CL_BLUE, "<...>: #%u\n", cli.id);
                colored_print(CL_YELLOW, "\tState: Open\n");
            
            }
            else if (cli.state == Client::COMPLETE) {
                colored_printf(CL_BLUE, "%s: #%u\n", cli.name, cli.id);
                colored_print(CL_GREEN, "\tState: Completed\n");
            }
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
        *host_client = Client(&server_addr, tcp_socket, cli_id++);
        get_comp_name((TCHAR*)host_client->name);
        host_client->state = Client::COMPLETE;
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
    for (;;) {
        while (!init_socket()) {
            P_ERROR("Socket connection failed, retrying...");
            Sleep(2000);
        }

        u32 s = state.load(std::memory_order_relaxed);

        if (s == IS_SERVER)
            conn_thread = std::thread(&Network::server_loop, this);
        else if (s == IS_CLIENT)
            conn_thread = std::thread(&Network::cli_loop, this);

        conn_thread.join();

        if (state.load(std::memory_order_relaxed) == FAILED_CONNECTION)
            cleanup();
    }
}

void Network::cli_loop()
{
    Msg msg = {};
    msg.hdr.type = Msg::NAME_SEND;
    std::strcpy((char*)msg.buffer, "Maks");
    assert(send(tcp_socket, (char*)&msg, sizeof(Msg), 0) != SOCKET_ERROR);

    assert(recv(tcp_socket, (char*)&msg, sizeof(Msg), MSG_WAITALL) != SOCKET_ERROR);

    for (int i = 0; i < msg.list.client_count; i++)
        printf("==>%s %d\n", msg.list.clients[i].name, msg.list.clients[i].id);


    exit(1);
}

void Network::server_analize_msg(const Msg& msg, socket_t socket)
{
    static Msg temp_msg_buf = {};
    memset(&temp_msg_buf, 0, sizeof(Msg));

    // Find client in list
    auto cli = std::find_if(
        std::begin(client_list),
        std::end(client_list),
        [&] (const Client& c) { return c.socket == socket; }
    );

    switch (msg.hdr.type)
    {
        case Msg::INVALID: 
            break;

        case Msg::NAME_SEND: {
            std::wcsncpy(cli->name, (wchar_t*)msg.buffer, CLIENT_NAME_LEN);
            cli->state = Client::COMPLETE;
            
            temp_msg_buf.hdr.type = Msg::CLIENT_LIST;
            u32 i = 0;
            for (const Client& c : client_list) {
                if (c.state == Client::COMPLETE) {
                    temp_msg_buf.list.clients[i].id = c.id;
                    std::wcsncpy(temp_msg_buf.list.clients[i].name, c.name, CLIENT_NAME_LEN);
                    ++i;
                }
            }
            temp_msg_buf.list.client_count = client_count;

            int sent_bytes = send(socket, (char*)&temp_msg_buf, sizeof(Msg), 0);

            if (sent_bytes <= 0) {
                LOG("Lost connection at send...\n");
                remove_client(socket);
                break;
            }

            assert(sent_bytes == sizeof(Msg));

            break;
        }

        default: break;
    }
}

#define EXIT_LOOP() { \
    state.store(FAILED_CONNECTION); \
    return; \
}

void Network::server_loop()
{   
    FD_ZERO(&master_fds);
    FD_SET(tcp_socket, &master_fds);

    for (;;) {
        work_fds = master_fds;
        
        const i32 res = select(0, &work_fds, NULL, NULL, NULL);

        if (res < 0) {
            P_ERROR("Error occured after select...\n");
            EXIT_LOOP();
        }

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
                    *new_client() = Client(&addr, client, cli_id++);
                }
            }
            else {
                Msg buf = {};

                i32 recv_bytes = recv(sock, (char*)&buf, sizeof(Msg), MSG_WAITALL);

                if (recv_bytes <= 0) {
                    LOG("Client disconnect\n");
                    remove_client(sock);
                }
                else {
                    assert(recv_bytes == sizeof(Msg));
                    server_analize_msg(buf, sock);
                    // debug_clients();
                }
            }
        }
    }
}

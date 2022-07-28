#include "network.hpp"
#include "quickshare.hpp"
#include <assert.h>
#include <algorithm>
#include <iterator>
#include "file_manager.hpp"

static void get_comp_name(TCHAR buffer[CLIENT_NAME_LEN])
{
    DWORD buf_size = CLIENT_NAME_LEN;
    GetComputerName(buffer, &buf_size);
}

Network::Network(File_Manager* f) : f_manager(f)
{
    temp_msg_buf = new Msg;
}

Network::~Network()
{
    delete temp_msg_buf;
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
}

Client* Network::new_client(struct sockaddr_in* addr, socket_t sock)
{
    auto cli = std::find_if(
            std::begin(client_list), 
            std::end(client_list),
            [] (const Client &c) { return c.state == Client::EMPTY; }
    );

    cli->id = get_id();
    cli->addr = *addr;
    cli->socket = sock;
    cli->state = Client::OPEN;
    ++client_count;

    LOGF("[%lld] Client created '%s:%d'\n", 
        cli->id,
        inet_ntoa(cli->addr.sin_addr), 
        cli->addr.sin_port);

    assert(cli != std::end(client_list));

    return cli;
}

UserId Network::get_id() const 
{
    UserId id;
    while (true) {
        id = time(NULL);

        if (std::find_if(
            std::begin(client_list), 
            std::end(client_list), 
            [&] (const Client& c) { 
                return c.id == id;
            }
        ) == std::end(client_list))
            break;
    }

    return id;
}

inline Client* Network::get_client(socket_t sock)
{
    auto cli = std::find_if(
        std::begin(client_list),
        std::end(client_list),
        [&] (const Client& c) { return c.socket == sock; }
    );

    if (cli == std::end(client_list))
        return NULL;
    else 
        return cli;
}

void Network::remove_client(socket_t sock)
{
    auto cli = get_client(sock);

    assert(cli);

    LOGF("[%lld] Client \"%s\" disconnected '%s:%d'\n", 
        cli->id, 
        (char*)cli->name, 
        inet_ntoa(cli->addr.sin_addr), 
        cli->addr.sin_port);
    
    closesocket(sock);
    FD_CLR(sock, &master_fds);
    std::memset(cli, 0, sizeof(Client));
    --client_count;

    // Send update client list if client_count is not equal to 1!!!
}

void Network::debug_clients() const
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
        
        Client* host_client = new_client(&server_addr, tcp_socket);
        assert(host_client);
        get_comp_name((TCHAR*)host_client->name);
        host_client->state = Client::COMPLETE;
        self = host_client;
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
    static Msg msg = {};
    int s_res, r_res;

    /*  SEND NAME                           */
    msg.hdr.type = Msg::NAME_SEND;
    std::strcpy((char*)msg.buffer, "Maks");
    s_res = send(tcp_socket, (char*)&msg, sizeof(Msg), 0);
    assert(s_res != SOCKET_ERROR && s_res == sizeof(Msg));

    /*  RECV USERS                           */
    r_res = recv(tcp_socket, (char*)&msg, sizeof(Msg), MSG_WAITALL);
    assert(r_res != SOCKET_ERROR);

    UserId other = msg.list.clients[0].id, my_id = msg.hdr.recipient_id;
    printf("User count: %d\n", msg.list.client_count);
    for (int i = 0; i < msg.list.client_count; i++)
        printf("==>%s %lld\n", (char*)msg.list.clients[i].name, msg.list.clients[i].id);


    /*  SEND REQUEST                           */
    msg.hdr.type = Msg::REQUEST;
    msg.hdr.recipient_id = msg.list.clients[0].id;
    msg.hdr.sender_id = my_id;

    assert(f_manager->set_send_hdr("text.txt", &msg.request));

    s_res = send(tcp_socket, (char*)&msg, sizeof(Msg), 0);
    assert(s_res != SOCKET_ERROR && s_res == sizeof(Msg));

    /*  RECEIVE ACCEPT                           */
    r_res = recv(tcp_socket, (char*)&msg, sizeof(Msg), MSG_WAITALL);
    assert(s_res != SOCKET_ERROR && s_res == sizeof(Msg));
    assert(msg.hdr.type == Msg::ACCEPTED);

    /*  FILE_MANAGER */
    memset((char*)&msg, 0, sizeof(Msg));
    msg.hdr.type = Msg::PACKET;
    msg.hdr.recipient_id = other;
    msg.hdr.sender_id = my_id;

    f_manager->set_state(Transfer_State::SENDING);
    f_manager->start_transfer();

    assert(f_manager->read(msg) == false);
    s_res = send(tcp_socket, (char*)&msg, sizeof(Msg), 0);
    assert(s_res != SOCKET_ERROR && s_res == sizeof(Msg));


    // while (f_manager->read(&msg)) {
    //     s_res = send(tcp_socket, (char*)&msg, sizeof(Msg), 0);
    //     assert(s_res != SOCKET_ERROR && s_res == sizeof(Msg));
    // }

    exit(1);
}

void Network::server_analize_msg(const Msg& msg, socket_t socket)
{
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

        case Msg::NAME_SEND:
            client_list_msg(msg, cli);
            break;

        case Msg::REQUEST:
            analize_request(msg, cli);
            break;

        case Msg::PACKET:
            analize_packet(msg, cli);
            break;

        default: 
            break;
    }
}

void Network::handle_packet(const Msg& msg, Client* cli)
{
    f_manager->write(msg);
}

void Network::analize_packet(const Msg& msg, Client* cli)
{
    // SAME AS anazlize request. either handle the message or redirect
    assert(msg.hdr.recipient_id == self->id);
    handle_packet(msg, cli);
}

void Network::send_client_list(Client* cli)
{
    // Echo to all new client list ::::TODO::::
    memset(temp_msg_buf, 0, sizeof(Msg));
    temp_msg_buf->hdr.type = Msg::CLIENT_LIST;
    temp_msg_buf->hdr.recipient_id = cli->id;
    u32 i = 0;
 
    for (const Client& c : client_list) {
        if (c.state == Client::COMPLETE && c.socket != cli->socket) {
            temp_msg_buf->list.clients[i].id = c.id;
            std::wcsncpy(temp_msg_buf->list.clients[i].name, c.name, CLIENT_NAME_LEN);
            ++i;
        }
    }
    temp_msg_buf->list.client_count = client_count - 1;

    int sent_bytes = send(cli->socket, (char*)temp_msg_buf, sizeof(Msg), 0);

    if (sent_bytes <= 0) {
        LOG("Lost connection at send...\n");
        remove_client(cli->socket);
        return;
    }

    assert(sent_bytes == sizeof(Msg));
}

void Network::handle_request(const Msg& msg, Client* cli)
{
    // Send message through queue to gui
    LOGF(
        "Transfer request:\n"
        "\tFrom:   [%lld] %s\n"
        "\tFile:  \"%s\"\n"
        "\tSize:   %u\n"
        "\tPacket: %u\n",
        cli->id, (char*)cli->name,
        msg.request.file_name,
        msg.request.file_size,
        msg.request.packets
    );

    if (f_manager->get_state() != Transfer_State::INACTIVE) {
        // Transfer must be rejected
        // send()
        LOG("TRANSFER REJECTED\n");
        return;
    }
    
    f_manager->set_recv_hdr(&msg.request);
    f_manager->set_state(Transfer_State::REQUESTED);

    /* Temp hard coded accept */
    LOG("SENDING ACCEPT\n");
    memset((char*)temp_msg_buf, 0, sizeof(Msg));
    temp_msg_buf->hdr.type = Msg::ACCEPTED;
    temp_msg_buf->hdr.sender_id = self->id;
    temp_msg_buf->hdr.recipient_id = cli->id;
    assert(send(cli->socket, (char*)temp_msg_buf, sizeof(Msg), 0) != SOCKET_ERROR);
    
    f_manager->set_state(Transfer_State::RECEVING);
    f_manager->start_transfer("recv.txt");
    //////////////////////////////////////////////////////////
}

void Network::analize_request(const Msg& msg, Client* cli)
{
    // Check if requests needs forwarding or if its for ther server aka client
    // Interface between gui and network, possibly a queue of messages
    // Once sent back accept message, consider a data structre for tracking 
    // started transfers
    // also manage the transfer / make sure they finish 
    // this can scale to multiple file transfers at once ^^
    //
    // for now tho make sure one can happen and that during it
    // all other requests are immidiately denied

    // Check if request is for the server or needs redirecting
    if (msg.hdr.recipient_id == self->id) {
        handle_request(msg, cli);
    }
    else {
        assert(false);
    }

}

void Network::client_list_msg(const Msg& msg, Client* cli)
{
    std::wcsncpy(cli->name, (wchar_t*)msg.buffer, CLIENT_NAME_LEN);
    cli->state = Client::COMPLETE;

    LOGF("Client init complete \"%s\"\n", (char*)cli->name);

    send_client_list(cli);
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
                    FD_SET(client, &master_fds);
                    (void)new_client(&addr, client);
                }
            }
            else {
                Msg buf = {};

                i32 recv_bytes = recv(sock, (char*)&buf, sizeof(Msg), MSG_WAITALL);

                if (recv_bytes <= 0) {
                    remove_client(sock);
                }
                else {
                    assert(recv_bytes == sizeof(Msg));
                    server_analize_msg(buf, sock);
                }
            }
        }
    }
}

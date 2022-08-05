#include "network.hpp"
#include "quickshare.hpp"
#include <assert.h>
#include "file_manager.hpp"
#include <cstring>

#define EXIT_LOOP() { \
    state.store(FAILED_CONNECTION, std::memory_order_relaxed); \
    return; \
}

static void get_comp_name(TCHAR buffer[CLIENT_NAME_LEN])
{
    DWORD buf_size = CLIENT_NAME_LEN;
    GetComputerName(buffer, &buf_size);
}

Network::Network(File_Sharing* f) : f_manager(f)
{
    temp_msg_buf = new Msg;
    db = new Database;
}

Network::~Network()
{
    cleanup();
    delete temp_msg_buf;
    delete db;
}

void Network::cleanup()
{
    LOG("Cleaning up server...\n");
	if (is_server)
    	FD_CLR(tcp_socket, &master_fds);
#ifdef SYSTEM_WIN_64
    closesocket(tcp_socket);
    WSACleanup();
#elif defined(SYSTEM_UNX)
    close(tcp_socket);
#endif
	if (is_server) {
		while (master_fds.fd_count > 0) {
			socket_t sock = master_fds.fd_array[0];
			FD_CLR(sock, &master_fds);
			closesocket(sock);
		}
	}
    db->cleanup();
    f_manager->cleanup();
}

void Network::close_connection(socket_t socket)
{
	File_Sharing::Transfer_State s = f_manager->r_data.state.load(std::memory_order_relaxed);

	if (s == File_Sharing::ACTIVE || s == File_Sharing::REQUESTED) {
		const Client* c = db->get_client(socket);
		for (const auto& u : f_manager->r_data.users) {
			if (u.first == c->id) {
				f_manager->fail_recv();
				break;
			}
		}
	}
	
	// Remove from database and close connection
	db->remove_client(socket);
	closesocket(socket);
	FD_CLR(socket, &master_fds);
	send_client_list();
}

bool Network::send_msg(const Msg* const msg, const Client* target)
{
    i32 sent_bytes = send(target->socket, (char*)msg, sizeof(Msg), 0);

    if (sent_bytes <= 0) {
        LOG("Lost connection during send...\n");
        close_connection(target->socket);
        return false;
    }

    assert(sent_bytes == sizeof(Msg));

	return true;
}

bool Network::send_to_id(Msg* msg, UserId to)
{
    if (is_server) {
        return send_msg(msg, db->get_client_by_id(to));
    }
    else {
        // Send to server
        i32 sent_bytes = send(tcp_socket, (char*)msg, sizeof(Msg), 0);
        if (sent_bytes <= 0) 
			return false;
        else 
			assert(sent_bytes == sizeof(Msg));
		return true;
    }
}

void Network::create_server_client()
{
    Client* const host_client = db->new_client(&server_addr, tcp_socket);
    get_comp_name((TCHAR*)host_client->name);
    host_client->state = Client::COMPLETE;
    this->my_id = host_client->id;
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
        is_server = false;
        try_connect(); // Wait till connection
    }
    else {
        LOG("Created server...\n");

        if (listen(tcp_socket, SOMAXCONN) < 0)
            return false;

        state.store(CONNECTED, std::memory_order_relaxed);
        is_server = true;
        create_server_client();
    }

    return true;
}

void Network::try_connect()
{
    do {
        if (connect(tcp_socket, 
                    (struct sockaddr*)&server_addr, 
                    sizeof(server_addr)) < 0) {
            state.store(FAILED_CONNECTION, std::memory_order_relaxed); 
            Sleep(2000);
        }
        else {
            LOG("Connected to server!\n");
            state.store(CONNECTED, std::memory_order_relaxed);
        }
    } while (state.load(std::memory_order_relaxed) == FAILED_CONNECTION);
}

void Network::network_loop()
{
    conn_thread = std::thread([&]() {
        for (;;) {
            while (!init_socket()) {
                P_ERROR("Socket connection failed, retrying...\n");
                Sleep(2000);
            }

            if (is_server)
                server_loop();
            else 
                cli_loop();

            if (state.load(std::memory_order_relaxed) == FAILED_CONNECTION)
                cleanup();
        }
    });
    conn_thread.detach();
}

void Network::cli_loop()
{
	// Send computers name
	temp_msg_buf->hdr.type = Msg::NAME_SEND;
	std::strcpy((char*)temp_msg_buf->buffer, "Maks");
	send_to_id(temp_msg_buf, 0);

	for (;;) {
		i32 r_res = recv(tcp_socket, (char*)temp_msg_buf, sizeof(Msg), MSG_WAITALL);

		if (r_res < 0) {
			P_ERROR("Lost connection to server...\n");
			EXIT_LOOP();
		}

		switch (temp_msg_buf->hdr.type)
		{
			case Msg::CLIENT_LIST: {
				db->client_count = temp_msg_buf->list.client_count;
				for (u32 i = 0; i < temp_msg_buf->list.client_count; i++) {
					db->client_list[i].state = Client::COMPLETE;
					db->client_list[i].id = temp_msg_buf->list.clients[i].id;
					std::wcsncpy(db->client_list[i].name, 
								temp_msg_buf->list.clients->name, 
								CLIENT_NAME_LEN);
				}

				my_id = temp_msg_buf->hdr.recipient_id;
				db->debug_clients();
				break;
			}

			case Msg::ACCEPTED: {
				if (temp_msg_buf->hdr.recipient_id != my_id) {
					P_ERRORF("Got invalid accept message hdr:\n"
							"\tFrom %lld\n\tTo %lld\n", 
							temp_msg_buf->hdr.sender_id,
							temp_msg_buf->hdr.recipient_id);
					break;
				}
				f_manager->got_response(temp_msg_buf);
				break;
			}
		}

	}

    // /*  SEND REQUEST                           */
    // Users_List a = {std::make_pair(other, Msg::INVALID)};
    // f_manager->create_send("text.txt", a);

	// /* TEST FAIL SEND */
	// // f_manager->fail_send();

    // /*  RECEIVE ACCEPT                           */
    // r_res = recv(tcp_socket, (char*)&msg, sizeof(Msg), MSG_WAITALL);
    // assert(s_res != SOCKET_ERROR && s_res == sizeof(Msg));
    // assert(msg.hdr.type == Msg::ACCEPTED);

    // // Find from who we recieved accept
    // f_manager->s_data.lock.lock();
    // auto& l = f_manager->s_data.users.front();
    // l.second = msg.hdr.type;
    // f_manager->s_data.lock.unlock();
    // f_manager->s_data.cond.notify_all();
    
    // getchar();
    // exit(1);
}

void Network::server_analize_msg(const Msg* msg, socket_t socket)
{
    Client* cli = db->get_client(socket);

    switch (msg->hdr.type)
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

void Network::handle_packet(const Msg* msg, Client* cli)
{
    // LOGF("Got packet:\n\t%s\n", (char*)msg->packet.bytes);
    f_manager->push_msg(msg);
}

void Network::analize_packet(const Msg* msg, Client* cli)
{
    // SAME AS anazlize request. either handle the message or redirect
    assert(msg->hdr.recipient_id == my_id);
    handle_packet(msg, cli);
}

void Network::send_client_list()
{
    // Echo message to all
    for (u32 i = 0; i < MAX_CLIENTS; i++) {
        const Client* c = &db->client_list[i];
        if (c->state == Client::COMPLETE && c->id != my_id) {
            db->create_msg(temp_msg_buf, c);
            send_msg(temp_msg_buf, c);
        }
    }
}

void Network::handle_request(const Msg* msg, Client* cli)
{
    // Send message through queue to gui
    LOGF(
        "Transfer request:\n"
        "\tFrom:   [%lld] %s\n"
        "\tFile:  \"%s\"\n"
        "\tSize:   %llu\n"
        "\tPacket: %llu\n",
        cli->id, (char*)cli->name,
        msg->request.file_name,
        msg->request.file_size,
        msg->request.packets
    );

    f_manager->create_recv(&msg->request, cli->id);
    f_manager->accept_request();

    // /* Temp hard coded accept */
    LOG("SENDING ACCEPT\n");
    memset((char*)temp_msg_buf, 0, sizeof(Msg));
    temp_msg_buf->hdr.type = Msg::ACCEPTED;
    temp_msg_buf->hdr.sender_id = my_id;
    temp_msg_buf->hdr.recipient_id = cli->id;
    assert(send(cli->socket, (char*)temp_msg_buf, sizeof(Msg), 0) != SOCKET_ERROR);
    //////////////////////////////////////////////////////////
}

void Network::analize_request(const Msg* msg, Client* cli)
{
    // Is the message directed at the server 
    if (msg->hdr.recipient_id == my_id) {
        handle_request(msg, cli);
    }
    else {
        // protection against fraudulant id ???
        send_msg(msg, db->get_client_by_id(msg->hdr.recipient_id));
    }
}

void Network::client_list_msg(const Msg* msg, Client* cli)
{
    // Complete client by adding name
    std::wcsncpy(cli->name, (wchar_t*)msg->buffer, CLIENT_NAME_LEN);
    cli->state = Client::COMPLETE;

    LOGF("Client init complete \"%s\"\n", (char*)cli->name);

    // Send list of all connected clients
    send_client_list();
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

                if (db->full()) {
                    LOGF("Client rejected '%s:%d'\n", inet_ntoa(addr.sin_addr), addr.sin_port);
                    closesocket(client);
                }
                else {
                    FD_SET(client, &master_fds);
                    (void)db->new_client(&addr, client);
                    // db->debug_clients();
                }
            }
            else {
                static Msg buf = {};

                i32 recv_bytes = recv(sock, (char*)&buf, sizeof(Msg), MSG_WAITALL);

                if (recv_bytes <= 0) {
                    LOG("Closing connection on recv...\n");
                    close_connection(sock);
                }
                else {
                    assert(recv_bytes == sizeof(Msg));
                    server_analize_msg(&buf, sock);
                }
            }
        }
    }
}

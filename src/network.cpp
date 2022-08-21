/** @file network.cpp
 * 
 * @brief Network module
 *      
 * Copyright (c) 2022 Maks S.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */ 

#include "network.hpp"
#include "quickshare.hpp"
#include <assert.h>
#include "file_manager.hpp"
#include <cstring>
#include "allocation.hpp"

#define EXIT_LOOP() { \
    state.store(FAILED_CONNECTION, std::memory_order_relaxed); \
    return; \
}

static void get_comp_name(char buffer[CLIENT_NAME_LEN])
{
    gethostname(buffer, CLIENT_NAME_LEN);
}

static void os_close_socket(socket_t sock)
{
#ifdef SYSTEM_WIN_64
    closesocket(sock);
#elif defined(SYSTEM_UNX)
    close(sock);
#endif
}

static void os_sleep(u32 sec)
{
#ifdef SYSTEM_WIN_64
    Sleep(sec * 1000);
#elif defined(SYSTEM_UNX)
    sleep(sec);
#endif
}

void Network::set_sock_timeout(u32 sec)
{
#ifdef SYSTEM_WIN_64
    DWORD timeout = sec * 1000;
    setsockopt(tcp_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
#endif
}

Network::Network(File_Sharing* f) : gui_msg(NETWORK_GUI_QUEUE_SIZE),
                                    f_manager(f)
{
    temp_msg_buf = memory.get_msg(0);
    db = memory.get_db();
}

Network::~Network()
{
    state.store(CLOSE, std::memory_order_relaxed);
    conn_thread.join();
    cleanup();
}

u32 Network::get_state() const
{
    return state.load(std::memory_order_relaxed);
}

void Network::cleanup()
{
    LOG("Cleaning up server...\n");
	if (is_server)
    	FD_CLR(tcp_socket, &master_fds);
#ifdef SYSTEM_WIN_64
    WSACleanup();
#endif
    os_close_socket(tcp_socket);
	if (is_server) {
		for (const auto& c : db->client_list)
            if (c.state != Client::EMPTY)
                os_close_socket(c.socket);
	}
    db->cleanup();
    f_manager->cleanup();
}

void Network::close_connection(socket_t socket)
{
    /* If connection that is being closed was in a transfer, safely 
       fail the file sharing recv loop 
    */
	File_Sharing::Transfer_State s = f_manager->r_data.state.load(std::memory_order_relaxed);
	if (s == File_Sharing::ACTIVE || s == File_Sharing::REQUESTED) {
		const Client* c = db->get_client(socket);
        // ? Might not need to be loop for r_data.users
		for (const auto& u : f_manager->r_data.users) {
			if (u.first == c->id) {
				f_manager->set_recv(File_Sharing::CLOSE);
				break;
			}
		}
	}
	
	// Remove from database and close connection
	db->remove_client(socket);
	os_close_socket(socket);
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
			assert(sent_bytes == sizeof(Msg)); // TODO wrap in loop if this were to ever fail
		return true;
    }
}

void Network::create_server_client()
{
    Client* const host_client = db->new_client(&server_addr, tcp_socket);
    get_comp_name(host_client->name);
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

#ifdef SYSTEM_WIN_64
    if (tcp_socket == INVALID_SOCKET) 
        return false;
#elif defined(SYSTEM_UNX)
    if (tcp_socet < 0)
        return false;
#endif

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //  INADDR_ANY;
	server_addr.sin_port = htons(STATIC_SERVER_PORT);

    if (bind(tcp_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        LOG("Attempting to connect to server...\n");
        is_server = false;
        try_connect();
        set_sock_timeout(1);
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
            os_sleep(2);
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
                os_sleep(2);
            }

            if (is_server)
                server_loop();
            else 
                cli_loop();

            const auto s = state.load(std::memory_order_relaxed);
            if (s == FAILED_CONNECTION)
                cleanup();
            else if (s == CLOSE)
                return;
        }
    });
}

void Network::cli_loop()
{
	// Send computers name
	temp_msg_buf->hdr.type = Msg::NAME_SEND;
	std::strcpy((char*)temp_msg_buf->name, "Maks");
	(void)send_to_id(temp_msg_buf, 0);

	for (;;) {
		i32 r_res = recv(tcp_socket, (char*)temp_msg_buf, sizeof(Msg), MSG_WAITALL);

		if (r_res < 0) {
            // Handle timout on windows
            #ifdef SYSTEM_WIN_64
            if (WSAGetLastError() == WSAETIMEDOUT) {
                if (state.load(std::memory_order_relaxed) == CLOSE)
                    return;
                else 
                    continue;
            } 
            #endif
            else {
                P_ERROR("Lost connection to server...\n");
                EXIT_LOOP();
            }
		}

		switch (temp_msg_buf->hdr.type)
		{
			case Msg::CLIENT_LIST: {
				db->client_count = temp_msg_buf->list.client_count;
				for (u32 i = 0; i < temp_msg_buf->list.client_count; i++) {
					db->client_list[i].state = Client::COMPLETE;
					db->client_list[i].id = temp_msg_buf->list.clients[i].id;
					std::strncpy(db->client_list[i].name, 
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

            default: assert(false); break;
		}
	}
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
    (void)cli;
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
    Net_Gui_Msg gmsg(Msg::CLIENT_LIST);

    for (u32 i = 0; i < MAX_CLIENTS; i++) {
        const Client* c = &db->client_list[i];
        if (c->state == Client::COMPLETE && c->id != my_id) {
            db->create_msg(temp_msg_buf, c);
            send_msg(temp_msg_buf, c);
        }
    }

    gmsg.copy_list(&temp_msg_buf->list);

    if (!gui_msg.try_push(gmsg)) {
        P_ERROR("'gui_msg' push too slow\n");
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

    assert(f_manager->create_recv(&msg->request, cli->id));
    f_manager->accept_request();

    // /* Temp hard coded accept */
    LOG("SENDING ACCEPT\n");
    memset((char*)temp_msg_buf, 0, sizeof(Msg));
    temp_msg_buf->hdr.type = Msg::ACCEPTED;
    temp_msg_buf->hdr.sender_id = my_id;
    temp_msg_buf->hdr.recipient_id = cli->id;
    assert(send(cli->socket, (char*)temp_msg_buf, sizeof(Msg), 0) > 0);
    //////////////////////////////////////////////////////////
}

void Network::analize_request(const Msg* msg, Client* cli)
{
    if (msg->hdr.recipient_id == my_id) {
        handle_request(msg, cli);
    }
    else {
        const Client* const c = db->get_client_by_id(msg->hdr.recipient_id);
        if (c == NULL) {
            LOGF("Request is being send to unknown id '%lld'...\n", msg->hdr.recipient_id);
            return;
        }
        send_msg(msg, db->get_client_by_id(msg->hdr.recipient_id));
    }
}

void Network::client_list_msg(const Msg* msg, Client* cli)
{
    // Complete client by adding name
    std::strncpy(cli->name, (char*)msg->name, CLIENT_NAME_LEN);
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
    #ifdef SYSTEM_WIN_64
        timeval timeout = {1, 0};
    #endif
        
        const i32 nready = select(FD_SETSIZE, &work_fds, NULL, NULL, &timeout);

        if (nready < 0) {
            P_ERROR("Error occured after select...\n");
            EXIT_LOOP();
        }
        else if (nready == 0) {
            if (state.load(std::memory_order_relaxed) == CLOSE)
                return;
        }

        for (i32 i = 0; i < nready; i++) {
            if (FD_ISSET(tcp_socket, &work_fds)) {
                accept_client();
            }
            else {
                for (const auto& c : db->client_list) {
                    if (c.state != Client::EMPTY && FD_ISSET(c.socket, &work_fds))
                        recv_msg(c.socket);
                }
            }
        }
    }
}

void Network::recv_msg(socket_t sock)
{
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

void Network::accept_client()
{
    struct sockaddr_in addr = {};
    int len = sizeof(addr);

    socket_t client = accept(tcp_socket, (struct sockaddr*)&addr, (socklen_t*)&len);

    if (db->full()) {
        LOGF("Client rejected '%s:%d'\n", inet_ntoa(addr.sin_addr), addr.sin_port);
        os_close_socket(client);
    }
    else {
        FD_SET(client, &master_fds);
        (void)db->new_client(&addr, client);
        // db->debug_clients();
    }
}
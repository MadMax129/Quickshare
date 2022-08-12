/** @file network.hpp
 * 
 * @brief Network definition
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

#pragma once

#include "quickshare.hpp"
#ifdef SYSTEM_WIN_64
#   include <WS2tcpip.h>
#   include <winsock2.h>
#elif defined(SYSTEM_UNX)
#   include <sys/socket.h>
#   include <unistd.h>
#   include <arpa/inet.h>
#endif
#include "msg.hpp"
#include <atomic>
#include <thread>

struct Client {
    enum {
        EMPTY,
        OPEN,
        COMPLETE
    } state;
    UserId id;
    char name[CLIENT_NAME_LEN];
    struct sockaddr_in addr;
    socket_t socket;
};

struct Database {
    Database();

    void cleanup();
    inline bool full() const { return client_count == MAX_CLIENTS; }
    UserId get_id() const;
    Client* new_client(struct sockaddr_in* addr, socket_t sock);
    void remove_client(socket_t sock);
    Client* get_client(socket_t sock);
    Client* get_client_by_id(UserId id);
    void debug_clients() const;
    void create_msg(Msg* msg, const Client* cli);
    std::array<Client, MAX_CLIENTS> client_list;
    u32 client_count;
};

/* Arbitrary server port that defines the central server location on local network */
#define STATIC_SERVER_PORT 8345

struct File_Sharing;
struct Client;
struct Database;

struct Network {
public:

    Network(File_Sharing* f);
    ~Network();

    /* Initialize network socket and return sucess */
    bool init_socket();

    /* Loop function interface */
    void network_loop();

    /* Close socket */
    void close_connection(socket_t socket);

    /* Send message api call */
    bool send_to_id(Msg* msg, UserId to);

    /* Get network's state */
    u32 get_state() const;

    /* Id of network instance */
    UserId my_id;
    
    enum {
        CLOSE,
        INACTIVE,
        FAILED_CONNECTION,
        CONNECTED
    };

private:
    /* Clean up all network resources */
    void cleanup();

    /* Attempts to connect to server, exits only if connected */
    void try_connect();
    
    /* Main threads for client or server networks */
    void cli_loop();
    void server_loop();
    
    /* Analize message send from clients */
    void server_analize_msg(const Msg* msg, socket_t socket);
    void client_list_msg(const Msg* msg, Client* cli);
    void send_client_list();
    void analize_request(const Msg* msg, Client* cli);
    void analize_packet(const Msg* msg, Client* cli);
    void accept_client();
    void recv_msg(socket_t sock);

    /* Client message handlers */
    void handle_request(const Msg* msg, Client* cli);
    void handle_packet(const Msg* msg, Client* cli);

    /* Debug / Util */
    bool send_msg(const Msg* const msg, const Client* target);
    void create_server_client();

    bool is_server;
#ifdef SYSTEM_WIN_64
    WSAData wsa_data;
#endif
    Database* db;
    File_Sharing* const f_manager;
    fd_set master_fds, work_fds;
    socket_t tcp_socket;
    Msg* temp_msg_buf;
    sockaddr_in server_addr;
    std::atomic<u32> state{INACTIVE};
    std::thread conn_thread;
};

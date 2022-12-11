#pragma once

class Network;
class Client;

#include "connection.hpp"
#include "thread_manager.hpp"
#include "database.hpp"

class Server {
public:
    Server(Network &net, const Client* cli);
    void init();
    void loop(Status& status);
    void cleanup();

private:
    void accept_client();
    void read_msgs();
    void init_server_db();
    void recv_msg(const socket_t sock);
    void close_client(const socket_t sock);
    void send_delete_client(const Slot& client_slot);
    bool is_to_server(const Server_Msg& msg);
    void send_to_client(const Server_Msg& msg);

    /* Server Messages */
    void analize_msg(const socket_t sock, Server_Msg& msg);
    void init_req(const socket_t sock, Server_Msg& msg);
    void send_client_list(const socket_t sock, Server_Msg& msg);

    fd_set master, worker;
    Network &net;
    const Client* cli;
    Database& db;
    const UserId my_id;
};
#pragma once

#include "msg.hpp"

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
#pragma once

#include <array>
#include "util.hpp"
#include "connection.hpp"
#include "config.hpp"

struct Slot {
    enum {
        /* Slot unused */
        EMPTY,
        /* Slot occupied by client, incomplete */
        OPENED,
        /* Active client under slot */
        COMPLETE
    } state;

    /* Unique id but also user created timestamp 
        used for determining dead users 
    */
    UserId id;
    char name[CLIENT_NAME_LEN];
    struct sockaddr_in addr;
    socket_t socket;
};

using Client_List = std::array<Slot, MAX_CLIENTS>;

class Database {
    Database();

    inline bool full() const { return client_count == MAX_CLIENTS; }

    void cleanup();
    UserId get_id() const;

private:
    Client_List client_list;
    u32 client_count;
};
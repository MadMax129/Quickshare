#pragma once

#include <array>
#include "util.hpp"
#include "connection.hpp"
#include "config.hpp"
#include <functional>

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
    socket_t sock;
};

using Client_List = std::array<Slot, MAX_CLIENTS>;

class Database {
public:
    Database();

    inline bool full() const { return client_count == MAX_CLIENTS; }
    inline u32 size() const { return client_count; };

    void iterate(std::function<void(Slot&)> const& func)
    {
        std::for_each(
            std::begin(client_list),
            std::end(client_list),
            func
        );
    }

    void new_client(const sockaddr_in* addr, const socket_t sock);
    void complete_client(const socket_t sock, 
                         const char name[CLIENT_NAME_LEN], 
                         const UserId id);
    const Slot* get_client(const socket_t sock);
    void remove_client(const socket_t sock);
    
    void cleanup();
    UserId get_id() const;

private:
    Client_List client_list;
    u32 client_count;
};
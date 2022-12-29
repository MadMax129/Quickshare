#pragma once

#include <array>
#include "util.hpp"
#include "connection.hpp"
#include "config.hpp"
#include <functional>
#include <mutex>
#include <atomic>
#include <vector>

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
    sockaddr_in addr;
    socket_t sock;
};

/* Minimized copy of a slot */
struct GUI_Slot {
    GUI_Slot(UserId id, const char name[]) {
        this->id = id;
        this->selected = false;
        safe_strcpy(this->name, name, CLIENT_NAME_LEN);
    }

    UserId id;
    char name[CLIENT_NAME_LEN];
    bool selected;
};

using Client_GUI_List = std::vector<GUI_Slot>;
using Client_List = std::array<Slot, MAX_CLIENTS>;

class Database {
public:
    Database();

    inline bool full() const { return client_count == MAX_CLIENTS; }
    inline u32 size() const { return client_count; };
    inline const Client_List& get_clist() const { return client_list; }

    void iterate(std::function<void(Slot&)> const& func);
    void new_client(const sockaddr_in* addr, const socket_t sock);
    void complete_client(const socket_t sock, 
                         const char name[CLIENT_NAME_LEN], 
                         const UserId id);
    void get_client(Slot& slot, const socket_t sock);
    void remove_client(const socket_t sock);
    
    void cleanup();
    UserId get_id() const;

    /* GUI Interface */
    inline u32 get_update() const { 
        return update.load(std::memory_order_relaxed);
    }

    inline void got_update(u32 count) {
        update.fetch_sub(count);
    }

    inline std::mutex& lock() {
        return mtx;
    }

    void copy(Client_GUI_List& list);

private:
    Client_List client_list;
    u32 client_count;
    std::mutex mtx;
    std::atomic<u32> update;
};
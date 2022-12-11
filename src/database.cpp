#include "database.hpp"
#include <algorithm>
#include <iterator>

Database::Database() 
{
    cleanup();
}

void Database::cleanup()
{
    std::lock_guard<std::mutex> guard(mtx);
    client_list.fill({});
    client_count = 0;
    update.store(0, std::memory_order_relaxed);
}

UserId Database::get_id() const 
{
    UserId id;
    while (true) {
        id = time(NULL);

        if (std::find_if(
            std::begin(client_list), 
            std::end(client_list), 
            [&] (const Slot& c) { 
                return c.id == id;
            }
        ) == std::end(client_list))
            break;
    }

    return id;
}

void Database::iterate(std::function<void(Slot&)> const& func)
{
    std::for_each(
        std::begin(client_list),
        std::end(client_list),
        func
    );
}

void Database::new_client(const sockaddr_in* addr, const socket_t sock)
{
    assert(!full());
    std::lock_guard<std::mutex> guard(mtx);

    auto slot = std::find_if(
        std::begin(client_list), 
        std::end(client_list),
        [] (const Slot &s) { 
            return s.state == Slot::EMPTY; 
        }
    );

    LOGF("Client Accepted %s:%d [%d]\n", 
        inet_ntoa(addr->sin_addr), 
        addr->sin_port,
        slot - client_list.cbegin()
    );

    slot->id = get_id();
    slot->addr = *addr;
    slot->sock = sock;
    slot->state = Slot::OPENED;
    ++client_count;
}

void Database::get_client(Slot& slot, const socket_t sock)
{
    const auto client = std::find_if(
        std::begin(client_list),
        std::end(client_list),
        [&] (const Slot& s) {
            return s.sock == sock;
        }
    );

    slot = *client;
}

void Database::complete_client(const socket_t sock, 
                               const char name[CLIENT_NAME_LEN], 
                               const UserId id)
{
    std::lock_guard<std::mutex> guard(mtx);
    auto slot = std::find_if(
        std::begin(client_list),
        std::end(client_list),
        [&] (const Slot& s) {
            return s.sock == sock;
        }
    );

    /* Client should already be accepted */
    assert(slot != std::end(client_list));

    slot->state = Slot::COMPLETE;
    safe_strcpy(slot->name, name, CLIENT_NAME_LEN);
    slot->id = id;
    
    LOGF("Client Complete %s:%d\n\tName: %s\n\tUserId: %lld\n", 
        inet_ntoa(slot->addr.sin_addr), 
        slot->addr.sin_port,
        name, 
        slot->id
    );
    update.fetch_add(1, std::memory_order_relaxed);
}

void Database::remove_client(const socket_t sock)
{
    std::lock_guard<std::mutex> guard(mtx);
    auto slot = std::find_if(
        std::begin(client_list),
        std::end(client_list),
        [&] (const Slot& s) {
            return s.sock == sock;
        }
    );

    if (slot == std::end(client_list)) {
        P_ERROR("Removing non-existent client\n");
        return;
    }

    LOGF("Client '%s' disconnected %s:%d\n",
        slot->state == Slot::OPENED ? "" : slot->name,
        inet_ntoa(slot->addr.sin_addr), 
        slot->addr.sin_port
    );

    slot->state = Slot::EMPTY;
    --client_count;
    update.fetch_add(1, std::memory_order_relaxed);
}

void Database::copy(Client_GUI_List& list)
{
    list.clear();

    for (const Slot& slot : client_list) {
        if (slot.state == Slot::COMPLETE)
            list.emplace_back(slot.id, slot.name);
    }
}
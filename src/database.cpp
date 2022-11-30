#include "database.hpp"
#include <algorithm>
#include <iterator>

Database::Database() 
{
    cleanup();
}

void Database::cleanup()
{
    client_list.fill({});
    client_count = 0;
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

void Database::new_client(const sockaddr_in* addr, const socket_t sock)
{
    assert(!full());

    auto slot = std::find_if(
        std::begin(client_list), 
        std::end(client_list),
        [] (const Slot &s) { 
            return s.state == Slot::EMPTY; 
        }
    );

    LOGF("Client Accepted %s:%d\n", inet_ntoa(addr->sin_addr), addr->sin_port);

    slot->id = get_id();
    slot->addr = *addr;
    slot->sock = sock;
    slot->state = Slot::OPENED;
    ++client_count;
}

const Slot* Database::get_client(const socket_t sock)
{
    const auto slot = std::find_if(
        std::begin(client_list),
        std::end(client_list),
        [&] (const Slot& s) {
            return s.sock == sock;
        }
    );

    return slot;
}

void Database::complete_client(const socket_t sock, 
                               const char name[CLIENT_NAME_LEN], 
                               const UserId id)
{
    auto slot = std::find_if(
        std::begin(client_list),
        std::end(client_list),
        [&] (const Slot& s) {
            return s.sock == sock;
        }
    );

    // Client should already be accepted
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
}

void Database::remove_client(const socket_t sock)
{
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
}
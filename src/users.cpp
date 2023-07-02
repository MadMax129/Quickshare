#include <cstring>
#include <algorithm>

#include "users.hpp"
#include "util.h"

User_List::User_List()
{
    dirty.store(false, std::memory_order_release);
}

User::User(const char name[PC_NAME_MAX_LEN],
           Client_ID id) : id(id)
{
    (void)std::memcpy(this->name, name, PC_NAME_MAX_LEN);
}

void User_List::add_users(const Packet* packet)
{
    assert(packet->hdr.type == P_SERVER_NEW_USERS);

    lock.lock();

    const auto& users = packet->d.new_users;

    for (i32 i = 0; i < users.users_len; i++) {
        user_list.emplace_back(
            users.names[i],
            users.ids[i]
        );
    }

    dirty.store(true, std::memory_order_release);
    lock.unlock();
}

void User_List::remove_user(const Packet* packet)
{
    assert(packet->hdr.type == P_SERVER_DEL_USERS);

    lock.lock();

    const auto& user = packet->d.del_user;

    const auto it = std::find_if(
        user_list.begin(), 
        user_list.end(), 
        [&](const User& u) {
            return u.id == user.id;
        }
    );

    if (it != user_list.end())
        user_list.erase(it);

    dirty.store(true, std::memory_order_release);

    lock.unlock();
}
#pragma once

#include <mutex>
#include <vector>
#include <atomic>

#include "config.h"
#include "msg.h"

struct User {
    User(const char name[PC_NAME_MAX_LEN],
         Client_ID id);

    Client_ID id;
    char name[PC_NAME_MAX_LEN];
    /* For GUI */
    bool selected;
};

using User_Vec = std::vector<User>;

struct User_List {
    static User_List& get_instance() {
        static User_List instance;
        return instance;
    }

    void add_users(const Packet* packet);
    void remove_user(const Packet* packet);
    
    inline void copy(User_Vec& vec)
    {
        if (dirty.load(std::memory_order_acquire)) {
            if (lock.try_lock()) {
                vec = user_list;
                dirty.store(false, std::memory_order_release);
                lock.unlock();
            }
        }
    }
    
private:
    User_List();
    User_List(const User_List&) = delete;
    User_List& operator=(const User_List&) = delete;

    std::mutex lock;
    std::atomic<bool> dirty;
    User_Vec user_list;
};
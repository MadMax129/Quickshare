#pragma once

#include "network.hpp"
#include "quickshare.hpp"
#include <cstring>

#define AMOUNT_OF_MSGS 3

struct Database;
struct Msg;

struct Allocation {
    enum {
        NETWORK_MSG,
        RECV_MSG,
        FILE_SHARING_MSG,
        DATABASE
    };

    bool try_allocate()
    {
        Msg* const msgs = (Msg*)std::malloc(sizeof(Msg) * AMOUNT_OF_MSGS);
        database = (Database*)std::malloc(sizeof(Database));

        if (!msgs || !database) {
            P_ERROR("Failed to allocate buffers\n");
            return false;
        }

        std::memset(msgs, 0, sizeof(Msg) * AMOUNT_OF_MSGS);
        *database = Database();

        msg_buf[NETWORK_MSG] = msgs;
        msg_buf[RECV_MSG] = msgs + sizeof(Msg);
        msg_buf[FILE_SHARING_MSG] = msgs + sizeof(Msg) * 2;

        return true;
    }

    Database* const get()
    {
        return database;
    }

    Msg* const get(u32 buffer)
    {
        assert(buffer < AMOUNT_OF_MSGS);
        return msg_buf[buffer];
    }

    void free()
    {
        if (msg_buf[0])
            std::free(msg_buf[0]);

        if (database)
            std::free(database);
    }

private:
    Msg* msg_buf[AMOUNT_OF_MSGS] = {NULL, NULL, NULL};
    Database* database = NULL;
};
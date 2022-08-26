#pragma once

#include "msg.hpp"
#include "database.hpp"
#include "config.hpp"
#include <cstring>
#include <new>

struct Allocation {
    enum {
        NETWORK_MSG,
    };

    bool try_allocate()
    {
        msgs = new (std::nothrow) Msg[AMOUNT_OF_MSGS];
        database = new (std::nothrow) Database;
        buffers = new (std::nothrow) u8[MEGABYTE * AMOUNT_OF_BUFFERS];

        if (!msgs || !database || !buffers) {
            P_ERROR("Failed to allocate buffers\n");
            return false;
        }

        std::memset(buffers, 0, MEGABYTE * AMOUNT_OF_BUFFERS);
        std::memset(msgs, 0, sizeof(Msg) * AMOUNT_OF_MSGS);

        return true;
    }

    Database* get_db() const
    {
        return database;
    }

    Msg* get_msg(u32 buffer) const
    {
        assert(buffer < AMOUNT_OF_MSGS);

        return msgs + buffer;
    }

    u8* get_buffer(u32 buffer) const
    {
        assert(buffer < AMOUNT_OF_BUFFERS);
        return buffers + MEGABYTE * buffer;
    }

    void free()
    {
        if (msgs) delete[] msgs;
        if (database) delete database;
        if (buffers) delete[] buffers;
    }

private:
    Msg* msgs = NULL;
    Database* database = NULL;
    u8* buffers = NULL;
};

extern Allocation memory;
#pragma once

#include "config.hpp"
#include "util.hpp"

#define SERVER_MSG_PACKET_SIZE 64 

struct Server_Msg {
    enum Type : u8 {
        /* Client init request */
        INIT_REQUEST,
        /* Server init response */
        INIT_RESPONSE,
    };

    Server_Msg() = default;
    inline Server_Msg(Type type) : type(type) {}

    UserId from, to;

    union {
        struct {
            char client_name[CLIENT_NAME_LEN];
        } init_req;
    };

    Type type;

} __attribute__((packed));
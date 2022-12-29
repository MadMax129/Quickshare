#pragma once

#include "config.hpp"
#include "util.hpp"

#define SERVER_MSG_PACKET_SIZE 512
#define FILE_NAME_LEN 256

struct Server_Msg {
    enum Type : u8 {
        /* Client init request */
        INIT_REQUEST,
        /* Server init response */
        INIT_RESPONSE,
        /* New client update */
        NEW_CLIENT,
        /* Disconnected client */
        DELETE_CLIENT,
        /* File share request */
        FILE_SHARE_REQ,
    };

    Server_Msg() = default;
    inline Server_Msg(Type type) : type(type) {}

    union {
        struct {
            char client_name[CLIENT_NAME_LEN];
        } init_req;

        struct {
            char client_name[CLIENT_NAME_LEN];
            UserId id;
        } cli_update;

        struct {
            UserId from, to;
        } response;

        struct {
            UserId from, to;
            u64 file_size;
            char file_name[FILE_NAME_LEN]; 
        } f_share_req;
    
    } d;

    Type type;

    char padding[SERVER_MSG_PACKET_SIZE - 
                sizeof(d) - 
                sizeof(Type)];

} __attribute__((packed));

static_assert(sizeof(Server_Msg) == SERVER_MSG_PACKET_SIZE);

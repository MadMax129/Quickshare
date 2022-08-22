#pragma once

#include <ctime>
#include "util.hpp"
#include "config.hpp"

#define MSG_TYPE(msg, mtype) msg->hdr.type = mtype
#define MSG_SENDER(msg, id) msg->hdr.sender_id = id

typedef time_t UserId;

struct Request {
    u64 file_size;
    u64 packets;
    char file_name[MAX_FILE_NAME];
};

struct Msg {
    enum Msg_Type : u8 {
        /* Empty message */
        INVALID,
        /* Server/client response for request */
        REJECTED, 
        /* Server/client response for request */
        ACCEPTED, 
        /* Packet */
        PACKET,
        /* Completes the client initialization */
        NAME_SEND,
        /* Echos client list and id's */
        CLIENT_LIST,
        /* Request sending a file / folder */
        REQUEST
    };

    struct {
        UserId recipient_id;
        UserId sender_id;
        Msg_Type type;
    } hdr;

    struct Client_List {
        struct {
            UserId id;
            char name[CLIENT_NAME_LEN];
        } clients[MAX_CLIENTS];
        u8 client_count;
    };

    union {
        Client_List list;
        Request request;

        struct {
            u16 packet_size; // [1, sizeof(bytes)]
            u8 bytes[PACKET_MAX_SIZE - sizeof(hdr) - sizeof(u16)];
        } packet;
        char name[CLIENT_NAME_LEN];

        u8 buffer[PACKET_MAX_SIZE - sizeof(hdr)];
    };
} __attribute__((packed));

static_assert(sizeof(Msg) == PACKET_MAX_SIZE);

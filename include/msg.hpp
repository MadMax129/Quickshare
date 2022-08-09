#pragma once

#include "quickshare.hpp"
#include <ctime>

/* Defines the size of a packet send through the network. */
#define PACKET_MAX_SIZE 1304

/* Max file name */
#define MAX_FILE_NAME 64

/* Defines the max number of clients accepted */
#define MAX_CLIENTS 16

/* Max length for display computer host name */
#define CLIENT_NAME_LEN 16

#ifdef SYSTEM_WIN_64
    typedef SOCKET socket_t;
#elif defined(SYSTEM_UNX)
    typedef int socket_t;
#endif

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

    union {
        struct {
            struct {
                UserId id;
                char name[CLIENT_NAME_LEN];
            } clients[MAX_CLIENTS];
            u8 client_count;
        } list;

        struct Request request;

        struct {
            u16 packet_size; // [1, sizeof(bytes)]
            u8 bytes[PACKET_MAX_SIZE - sizeof(hdr) - sizeof(u16)];
        } packet;
        char name[CLIENT_NAME_LEN];

        u8 buffer[PACKET_MAX_SIZE - sizeof(hdr)];
    };
} __attribute__((packed));

static_assert(sizeof(Msg) == PACKET_MAX_SIZE);

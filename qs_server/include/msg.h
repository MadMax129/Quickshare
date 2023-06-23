#ifndef QS_MSG
#define QS_MSG

#include <stdint.h>
#include "config.h"

enum {
    /* Client intro message
       setup session
       + SERVER_OK
       - SERVER_DENY
    */
    P_SERVER_OK,
    P_SERVER_DENY,
    P_CLIENT_INTRO,
    
    /* Server reply to client
       session connection with users
    */
    P_SERVER_NEW_USERS,
    P_SERVER_DEL_USERS,

    /* Transfer file msgs
        Request (C)
            * request
            + Valid (transfer_info)
            - Invalid
        Reply (S/C)
            * transfer_reply
            + Valid
            - Invalid
        Data (S/C)
        Cancel (S/C)
        Complete (S/C) 
        Valid (S)
        Invalid (S)
    */
   P_TRANSFER_VALID,
   P_TRANSFER_INVALID,
   P_TRANSFER_REQUEST,
   P_TRANSFER_REPLY,
   P_TRANSFER_DATA,
   P_TRANSFER_CANCEL,
   P_TRANSFER_COMPLETE
};

#define PACKET_HDR(ptype, psize, packet) { \
    packet->hdr.type = ptype; \
    packet->hdr.size = psize; \
}

typedef struct {
    uint32_t type;
    uint32_t size;
} Packet_Hdr;

typedef struct {
    Transfer_ID t_id;
    Client_ID from;
    Client_ID to[TRANSFER_CLIENTS_MAX];
} Transfer_Hdr;

typedef struct {
    Packet_Hdr hdr;
    union {
        char data[
            PACKET_MAX_SIZE - 
            sizeof(Packet_Hdr) - 
            sizeof(uint32_t)
        ];

        struct Intro {
            char name[PC_NAME_MAX_LEN];
            char id[SESSION_ID_MAX_LEN];
            /* 0 - Join | 1 - Create */
            uint8_t session;
            uint8_t name_len;
            uint8_t id_len;
        } intro;

        struct {
            char names[PC_NAME_MAX_LEN][CLIENT_LIST_LEN];
            time_t ids[CLIENT_LIST_LEN];
            uint8_t users_len;
        } users;

        struct {
            Transfer_Hdr hdr;
            char file_name[FILE_NAME_LEN];
            uint64_t file_size;
        } request;

        struct {
            Transfer_Hdr hdr;
            uint8_t accept;
        } transfer_reply;

        struct {
            Transfer_Hdr hdr;
        } transfer_state;

        struct {
            Transfer_ID id;
        } transfer_info;

        struct {
            Transfer_Hdr hdr;
            char bytes[
                PACKET_MAX_SIZE - 
                sizeof(Packet_Hdr) - 
                sizeof(uint32_t) -
                sizeof(Transfer_Hdr)
            ];
        } transfer_data;
    } d;
} __attribute__((packed)) Packet;

_Static_assert(
    sizeof(Packet) == PACKET_MAX_SIZE,
    "Packet not MTU size..."
);

#endif /* QS_MSG */
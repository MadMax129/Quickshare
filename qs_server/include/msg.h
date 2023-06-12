#ifndef QS_MSG
#define QS_MSG

#include <stdint.h>

#define MTU_SIZE 1440
#define PC_NAME 16
#define SESSION_ID 16

enum {
    /* Server OK reply 
       to client request 
    */
    P_SERVER_OK,

    /* Server DENY reply 
       to client request 
    */
    P_SERVER_DENY,

    /* Client intro message
       setup session
    */
    P_CLIENT_INTRO,
    
    /* Server reply to client
       session connection with users
    */
    P_SERVER_NEW_USERS,
    P_SERVER_DEL_USERS,
};

typedef struct {
    uint8_t type;
    uint16_t size;
} Packet_Hdr;

typedef struct {
    Packet_Hdr hdr;
    uint32_t : 32;
    union {
        char data[
            MTU_SIZE - 
            sizeof(Packet_Hdr) - 
            sizeof(uint32_t)
        ];

        struct {
            char name[PC_NAME];
            char id[SESSION_ID];
            /* 0 - Join | 1 - Create */
            uint8_t session;
            uint8_t name_len;
            uint8_t id_len;
        } intro;

        struct {
            char names[PC_NAME][32];
            time_t ids[32];
            uint8_t users_len;
        } users;
    } d;
} __attribute__((packed)) Packet;

_Static_assert(
    sizeof(Packet) == MTU_SIZE,
    "Packet not MTU size..."
);

#endif /* QS_MSG */
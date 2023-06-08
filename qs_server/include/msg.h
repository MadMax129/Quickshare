#ifndef QS_MSG
#define QS_MSG

#include <stdint.h>

#define MTU_SIZE 1440

// enum {

// };

typedef struct {
    struct Header {
        uint8_t type;
    } hdr;

    union {
        char data[
            MTU_SIZE - 
            sizeof(struct Header)
        ];
    } d;
} Packet;

_Static_assert(
    sizeof(Packet) == MTU_SIZE,
    "Packet not MTU size..."
);

#endif /* QS_MSG */
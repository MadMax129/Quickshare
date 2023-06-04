#ifndef QS_CLIENT
#define QS_CLIENT

#include <arpa/inet.h>
#include <stdbool.h>
#include <openssl/bio.h>
#include "ringbuffer.h"
#include "secure.h"
#include "msg.h"

#define MAX_CLIENTS 32

typedef struct {
    int fd;
    struct sockaddr_in addr;
    Secure secure;
    char* w_buf;
    unsigned int w_len;
    // Packet* packet;
    bool used;
} Client;

typedef struct {
    Client* list;
    unsigned int n_cli;
} Client_List;

void client_list_init(Client_List* clist);
void client_list_free(Client_List* clist);
void client_queue_write(Client* client, const char *buf, size_t len);
Client* client_init(Client_List* clist);
Client* client_find(Client_List* clist, int fd);
Client* client_close(Client_List* clist, int fd);

#endif /* QS_CLIENT */
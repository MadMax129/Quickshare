#ifndef QS_CLIENT
#define QS_CLIENT

#include <arpa/inet.h>
#include <stdbool.h>
#include <openssl/bio.h>
#include <time.h>

#include "ringbuffer.h"
#include "secure.h"
#include "msg.h"
#include "queue.h"
#include "database.h"

#define MAX_CLIENTS 32
#define MSG_QUEUE_SIZE 8
#define NAME_SIZE

typedef struct {
    time_t id;
    int fd;
    struct sockaddr_in addr;
    Secure secure;
    enum {
        /* Empty client slot */
        C_EMPTY,
        /* Connected */
        C_CONNECTED,
        /* Secured TLS */
        C_SECURE,
        /* Intro send and accepted */
        C_COMPLETE
    } state;
    Session_ID session_id;
    Queue msg_queue;
    char name[PC_NAME];
    Packet* p_buf;
    unsigned int p_len; 
} Client;

typedef struct {
    Client* list;
    unsigned int n_cli;
} Client_List;

void client_list_init(Client_List* clist);
void client_list_free(Client_List* clist);
Client* client_init(Client_List* clist);
Client* client_find(Client_List* clist, int fd);
Client* client_close(Client_List* clist, int fd);

#endif /* QS_CLIENT */
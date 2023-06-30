#ifndef QS_CLIENT
#define QS_CLIENT

#include <arpa/inet.h>
#include <stdbool.h>
#include <openssl/bio.h>
#include <time.h>

#include "secure.h"
#include "msg.h"
#include "queue.h"
#include "database.h"
#include "buffer.h"

#define MAX_CLIENTS 32
#define MSG_QUEUE_SIZE 8
#define NAME_SIZE

typedef struct {
    Client_ID id;
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
        /* Intro accepted */
        C_COMPLETE
    } state;
    Session_ID session_id;
    Queue msg_queue;
    char name[PC_NAME_MAX_LEN];
    Buffer read_buf, decrypt_buf;
} Client;

typedef struct {
    Client* list;
    unsigned int n_cli;
} Client_List;

void client_list_init(Client_List* clist);
void client_list_free(Client_List* clist);
Client* client_init(Client_List* clist);
Client* client_find(Client_List* clist, int fd);
Client* client_find_by_id(Client_List* clist, Client_ID id);
Client* client_close(Client_List* clist, int fd);

#endif /* QS_CLIENT */
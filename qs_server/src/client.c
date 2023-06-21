#include <stdlib.h>
#include <memory.h>
#include <time.h>

#include "client.h"
#include "server.h"
#include "die.h"
#include "mem.h"

void client_list_init(Client_List* clist)
{
    clist->list = (Client*)alloc(MAX_CLIENTS * sizeof(Client));

    if (!clist->list)
        die("Failed alloc clients");

    for (unsigned int i = 0; i < MAX_CLIENTS; i++) {
        clist->list[i].p_buf = (Packet*)alloc(sizeof(Packet));
        if (!clist->list[i].p_buf)
            die("Failed packet malloc");

        if (!queue_init(
                &clist->list[i].msg_queue, 
                sizeof(Packet), 
                MSG_QUEUE_SIZE))
            die("Failed queue_init");

        /* Secure buffer */
        clist->list[i].secure.e_buf  = (char*)alloc(sizeof(Packet) * 2);
        clist->list[i].secure.e_size = sizeof(Packet) * 2;
        if (!clist->list[i].secure.e_buf)
            die("secure buffer malloc");
    }

    clist->n_cli = 0;
}

void client_list_free(Client_List* clist)
{
    for (unsigned int i = 0; i < MAX_CLIENTS; i++) {
        Client* c = &clist->list[i];
        (void)c;
        // !free ssl stuff
    }
}

static void client_configure(Client* c)
{
    c->state = C_CONNECTED;
    c->id = time(NULL);
    memset(c->p_buf, 0, sizeof(Packet));
    c->p_len  = 0;
    c->p_size = 0;

    /* Init SSL */
    secure_init(&c->secure);
}

Client* client_init(Client_List* clist)
{
    if (clist->n_cli == MAX_CLIENTS)
        return NULL;

    for (unsigned int i = 0; i < MAX_CLIENTS; i++) {
        if (clist->list[i].state == C_EMPTY) {
            client_configure(&clist->list[i]);
            ++clist->n_cli;
            return &clist->list[i];
        }
    }

    return NULL;
}

Client* client_find(Client_List* clist, int fd)
{
    for (unsigned int i = 0; i < MAX_CLIENTS; i++) {
        if (
            clist->list[i].state != C_EMPTY &&
            clist->list[i].fd == fd
        )
            return &clist->list[i];
    }

    return NULL;
}

Client* client_find_by_id(Client_List* clist, Client_ID id)
{
    for (unsigned int i = 0; i < MAX_CLIENTS; i++) {
        if (
            clist->list[i].state != C_EMPTY &&
            clist->list[i].id == id
        )
            return &clist->list[i];
    }

    return NULL;
}

Client* client_close(Client_List* clist, int fd)
{
    Client* client = client_find(clist, fd);

    if (!client)
        return NULL;

    client->state = C_EMPTY;

    /* Shutdown SSL */
    secure_free(&client->secure);
    queue_reset(&client->msg_queue);
    --clist->n_cli;

    return client;
}
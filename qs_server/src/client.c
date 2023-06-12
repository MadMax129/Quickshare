#include <stdlib.h>
#include <memory.h>
#include <time.h>

#include "client.h"
#include "server.h"
#include "util.h"

void client_list_init(Client_List* clist)
{
    clist->list = calloc(MAX_CLIENTS, sizeof(Client));

    if (!clist->list)
        die("Failed alloc clients");

    for (unsigned int i = 0; i < MAX_CLIENTS; i++) {
        clist->list[i].p_buf = (Packet*)malloc(sizeof(Packet));
        if (!clist->list[i].p_buf)
            die("Failed packet malloc");

        if (!queue_init(
                &clist->list[i].msg_queue, 
                sizeof(Packet), 
                MSG_QUEUE_SIZE))
            die("Failed queue_init");
    }

    clist->n_cli = 0;
}

void client_list_free(Client_List* clist)
{
    for (unsigned int i = 0; i < MAX_CLIENTS; i++) {
        Client* c = &clist->list[i];
        free(c->p_buf);
        if (c->secure.e_buf)
            free(c->secure.e_buf);
        queue_free(&c->msg_queue);

        // !free secure
    }
    free(clist->list);
}

static void client_configure(Client* c)
{
    c->state = C_CONNECTED;
    c->id = time(NULL);
    memset(c->p_buf, 0, sizeof(Packet));
    c->p_len = 0;

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
        if (clist->list[i].fd == fd)
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
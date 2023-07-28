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
    clist->n_cli = 0;

    if (!clist->list)
        die("Failed alloc clients");

    for (unsigned int i = 0; i < MAX_CLIENTS; i++) {
        /* TCP read buffer and decrypt buffer */
        BUFFER_ALLOC(clist->list[i].read_buf, sizeof(Packet));
        BUFFER_ALLOC(clist->list[i].decrypt_buf, sizeof(Packet));
        
        /* Secure encrypt buffer */
        BUFFER_ALLOC(
            clist->list[i].secure.encrypted_buf,
            sizeof(Packet) * 2
        );

        if (!B_DATA(clist->list[i].read_buf)    ||
            !B_DATA(clist->list[i].decrypt_buf) ||
            !B_DATA(clist->list[i].secure.encrypted_buf)
        ) {
            die("Failed packet malloc");
        }

        /* Client Message Queue */
        if (!queue_init(
                &clist->list[i].msg_queue, 
                sizeof(Packet), 
                MAX_CLIENTS / 2)
        ) {
            die("Failed queue_init");
        }
    }
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
    c->state      = C_CONNECTED;
    c->id         = time(NULL);
    c->session_id = 0;
    c->fd         = 0;
    memset(c->name, 0, sizeof(c->name));

    /* Zero buffers */
    B_RESET(c->read_buf);
    B_RESET(c->decrypt_buf);

    /* Init SSL */
    secure_init(&c->secure);

    /* Reset Msg Queue */
    queue_reset(&c->msg_queue);
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
    --clist->n_cli;

    return client;
}
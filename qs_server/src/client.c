#include "client.h"
#include "server.h"
#include <stdlib.h>
#include <memory.h>

void client_list_init(Client_List* clist)
{
    clist->list = calloc(MAX_CLIENTS, sizeof(Client));

    if (!clist->list)
        die("Failed alloc clients");

    for (unsigned int i = 0; i < MAX_CLIENTS; i++) {
        clist->list[i].p_buf = (Packet*)malloc(sizeof(Packet));

        if (!clist->list[i].p_buf)
            die("Failed packet malloc");
    }

    clist->n_cli = 0;
}

void client_list_free(Client_List* clist)
{
    for (unsigned int i = 0; i < MAX_CLIENTS; i++) {
        free(clist->list[i].p_buf);
        // !free secure
    }
    free(clist->list);
}

Client* client_init(Client_List* clist)
{
    if (clist->n_cli == MAX_CLIENTS)
        return NULL;

    for (unsigned int i = 0; i < MAX_CLIENTS; i++) {
        if (clist->list[i].state == C_EMPTY) {
            clist->list[i].state = C_CONNECTED;
            clist->list[i].p_len = 0;

            secure_init(&clist->list[i].secure);
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
    --clist->n_cli;
    secure_free(&client->secure);
    memset(client->secure.encrypted_buf, 0, client->secure.e_len);

    return client;
}
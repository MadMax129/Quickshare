#include "client.h"
#include "server.h"
#include <stdlib.h>
#include <memory.h>

void client_list_init(Client_List* clist)
{
    clist->list = calloc(MAX_CLIENTS, sizeof(Client));

    if (!clist->list)
        die("Failed alloc clients");

    clist->n_cli = 0;
}

void client_queue_write(Client* client, const char *buf, size_t len)
{
    client->w_buf = (char*)realloc(client->w_buf, client->w_len + len);
    memcpy(client->w_buf + client->w_len, buf, len);
    client->w_len += len;
}

void client_list_free(Client_List* clist)
{
    for (unsigned int i = 0; i < MAX_CLIENTS; i++) {
        free(clist->list[i].w_buf);
        // free secure
    }
    free(clist->list);
}

Client* client_init(Client_List* clist)
{
    if (clist->n_cli == MAX_CLIENTS)
        return NULL;

    for (unsigned int i = 0; i < MAX_CLIENTS; i++) {
        if (!clist->list[i].used) {
            clist->list[i].used = true;
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

    client->used = false;
    --clist->n_cli;
    secure_free(&client->secure);
    memset(client->w_buf, 0, client->w_len);
    printf("Here\n");

    return client;
}
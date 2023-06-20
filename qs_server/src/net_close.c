#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "server.h"

static void send_user_disconnect(Server* s, Client* c)
{
    /* Inform other clients */
    for (int i = 0; i < MAX_CLIENTS; i++) {
        Client* other = &s->clients.list[i];
        if (other->id != c->id && 
            other->session_id == c->session_id &&
            other->state == C_COMPLETE)
        {
            send_single_user(
                other, 
                c,
                P_SERVER_DEL_USERS 
            );
        }
    }
}

static void cancel_transaction_creator(Server* s, Client* c, Transfer_ID t_id)
{
    /* Close all clients transfers */
    while (t_id) {
        printf("User created Transfer=%ld\n", t_id);
        // select * from TransferClients where transfer_id=t_id
        Client_ID transfer_client_id = db_get_client_all(&s->db, t_id);
        while (transfer_client_id) {
            printf(">>>>>>>>>>>>Participant %ld\n", transfer_client_id);
            // step send Cancel step ...
            transfer_client_id = db_client_all_step(&s->db);
        }

        // delete TransferClients where transfer_id=t_id
        // delete Transfer where transfer_id=t_id

        t_id = db_transfer_step(&s->db);
    }
}

static void cancel_transaction_user(Server* s, Client* c)
{

}

static void cancel_transaction(Server* s, Client* c)
{
    if (!db_transaction(&s->db, BEGIN_TRANSACTION))
        return;

    Transfer_ID t_id = db_get_transfer(&s->db, c->id);
    
    if (t_id != 0)
        cancel_transaction_creator(s, c, t_id);
    else 
        cancel_transaction_user(s, c);

    if (!db_transaction(&s->db, COMMIT_TRANSACTION))
        return;

}

void close_client(Server* s, int fd)
{
    Client* client = client_find(&s->clients, fd);
    
    if (!client) {
        printf("Error removing client\n");
        return;
    }

    send_user_disconnect(s, client);

    /* Cancel any transactions client may have created
        also send P_TRANSACTION_CANCEL to all transaction clients
    */
   cancel_transaction(s, client);

    (void)client_close(&s->clients, fd);

    printf("Client DISC [#%d, %d] %s:%d\n",
        (int)(client - s->clients.list), fd,
        inet_ntoa(client->addr.sin_addr), 
        client->addr.sin_port
    );

    assert(new_client_event(s, EPOLL_CTL_DEL, fd));
    close(client->fd);
}
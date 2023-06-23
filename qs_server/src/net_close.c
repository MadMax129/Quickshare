#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include "util.h"

#include "server.h"

static void send_user_disconnect(Server* s, Client* c)
{
    /* Inform other session clients */
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

static void send_transfer_info(Client* c, Client_ID from, 
                               Transfer_ID t_id, int type)
{
    Packet* packet = enqueue(&c->msg_queue);
    assert(packet);
    PACKET_HDR(
        type,
        sizeof(packet->d.transfer_state),
        packet
    );
    packet->d.transfer_state.hdr.t_id = t_id;
    packet->d.transfer_state.hdr.from = from;
}

static void cancel_transaction_creator(Server* s, Client* c, Transfer_ID t_id)
{
    /* Client is a transfer creator
        * Send TRANSFER_CANCEL to all participants
        * Delete all participants from TransferClient
        * Delete the transfer itself
    */
    do {
        Client_ID t_client_id = db_get_client_all(&s->db, t_id);
        while (t_client_id) {
            Client* recipient = client_find_by_id(&s->clients, t_client_id);
            if (recipient) {
                send_transfer_info(
                    recipient,
                    c->id,
                    t_id, 
                    P_TRANSFER_CANCEL
                );
            }
            t_client_id = db_client_all_step(&s->db);
        }
        db_cleanup_transfer(&s->db, t_id);
        t_id = db_transfer_step(&s->db);
    } while (t_id);
}

static void cancel_transaction_user(Server* s, Client* c)
{
    /* Client is a transfer reciever
        * Send TRANSFER_CANCEL to transfer creator
        * Remove client from TransferClients
    */
    Transfer_Info info = db_get_creator(&s->db, c->id);

    while (info.creator) {
        Client* creator = client_find_by_id(
            &s->clients, 
            info.creator
        );

        if (creator) {
            send_transfer_info(
                creator,
                c->id,
                info.t_id,
                P_TRANSFER_CANCEL
            );
        }
        info = db_creator_step(&s->db);
    }
    db_client_delete(&s->db, c->id);
}

static void cancel_transaction(Server* s, Client* c)
{
    if (!db_transaction(&s->db, BEGIN_TRANSACTION))
        return;

    Transfer_ID t_id = db_get_transfer(&s->db, c->id);
    
    if (t_id != 0)
        cancel_transaction_creator(s, c, t_id);
    
    cancel_transaction_user(s, c);

    (void)db_transaction(&s->db, COMMIT_TRANSACTION);
}

void close_client(Server* s, int fd)
{
    if (!new_client_event(s, EPOLL_CTL_DEL, fd)) {
        P_ERRORF("epoll event DEL error %d\n", fd);
        return;
    }

    Client* client = client_find(&s->clients, fd);
    
    if (!client) {
        P_ERRORF("Error removing client %d\n", fd);
        return;
    }

    if (client->state == C_COMPLETE) {
        cancel_transaction(s, client);
        send_user_disconnect(s, client);
    }
    
    LOGF("Client DISC [#%d:%d, %ld] %s:%d\n",
        (int)(client - s->clients.list), fd, client->id,
        inet_ntoa(client->addr.sin_addr), 
        client->addr.sin_port
    );

    (void)client_close(&s->clients, fd);
    (void)close(client->fd);
}
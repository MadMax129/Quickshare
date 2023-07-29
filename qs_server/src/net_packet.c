#include <string.h>
#include <assert.h>

#include "util.h"
#include "server.h"

/* 
 * Update connecting client on all clients in their session.
 * Also update existing client on the connecting client.
 * 
 * Only send if:
 *  - state is C_COMPLETE
 *  - in same session id
 */
static void send_session_users(Server* s, Client* c)
{
    Packet* packet = NULL;
    int count      = 0;

    for (unsigned int i = 0; i < MAX_CLIENTS; i++) 
    {
        Client* user = &s->clients.list[i];

        if (
            user->id != c->id         &&
            user->state == C_COMPLETE && 
            user->session_id == c->session_id
        ) {
            if (!packet) {
                packet = enqueue(&c->msg_queue);
                assert(packet);

                PACKET_HDR(
                    P_SERVER_NEW_USERS,
                    sizeof(packet->d.new_users),
                    packet
                );
            }

            memcpy(
                &packet->d.new_users.names[count],
                user->name,
                PC_NAME_MAX_LEN
            );
            packet->d.new_users.ids[count] = user->id;
            ++packet->d.new_users.users_len;
            ++count;

            send_single_user(user, c, P_SERVER_NEW_USERS);
        }
    }
}

static inline void server_resposne(Client* c, int type)
{
    Packet* packet = enqueue(&c->msg_queue);
    assert(packet);
    PACKET_HDR(type, 0, packet);
}

/* 
 * First message sent from client to the server signifing
 * creating itself as a client. 
 * 
 * Struct d.intro
 * - session (0 to create, 1 to join)
 * - name[PC_MAX_LEN] (client name)
 * - id[SESSION_ID_MAX_LEN] (session id)
 * 
 * Server Response:
 * P_SERVER_OK or P_SERVER_DENY
 */
static void packet_intro(Server* s, Client* c)
{
    Packet* const p = B_DATA(c->decrypt_buf);

    /* Make sure all data makes sense */
    if (c->state == C_COMPLETE ||
        p->d.intro.session > 1
    ) {
        /* Abnormal client behavior */
        P_ERROR("Abnormal client packet intro\n");
        server_resposne(c, P_SERVER_DENY);
        return;
    }

    p->d.intro.name[PC_NAME_MAX_LEN  - 1] = '\0';
    p->d.intro.id[SESSION_ID_MAX_LEN - 1] = '\0';
   
    SERVER_LOG(
        s,
        "********** Packet: Intro **********\n"
        "\tName:    '%s'\n"
        "\tSession: %s\n"
        "\tType:    %s\n"
        "***********************************\n",
        p->d.intro.name,
        p->d.intro.id,
        p->d.intro.session ? "Create" : "Join"
    );

    long session = 0;
    if (p->d.intro.session == 0)
        session = db_get_session(&s->db, p->d.intro.id);
    else if (p->d.intro.session == 1)
        session = db_create_session(&s->db, p->d.intro.id);

    if (session == 0) {
        server_resposne(c, P_SERVER_DENY);
        return;
    }

    server_resposne(c, P_SERVER_OK);
    c->state = C_COMPLETE;
    c->session_id = session;
    memcpy(
        c->name, 
        p->d.intro.name,
        PC_NAME_MAX_LEN
    );

    send_session_users(s, c);
}

/* 
 * Check if recipients exist and if so append them into
 * the table TransferClients.
 * 
 * Fail:
 *  - Recipients length is 0
 *  - Client ID is invalid
 */
static bool check_recipients(
    Server* s, 
    const Client* c, 
    const Transfer_ID t_id, 
    const Packet* p
)
{
    for (int i = 0; i < TRANSFER_CLIENTS_MAX; i++) {
        Client_ID c_id = p->d.request.hdr.to[i];

        if (c_id == 0)
            return i != 0;
        
        const Client* r = client_find_by_id(
            &s->clients,
            c_id
        );

        if (!r || 
            (r->session_id != c->session_id) || 
            !db_create_client(&s->db, t_id, r->id)
        ) {
            return false;
        }
    }

    return true;
}

static void echo_transfer_request(
    Server* s, 
    const Client* c, 
    const Packet* req, 
    const Transfer_ID t_id
)
{
    for (int i = 0; i < TRANSFER_CLIENTS_MAX; i++) {
        const Client_ID c_id = req->d.request.hdr.to[i];

        if (c_id == 0)
            break;

        Client* r = client_find_by_id(
            &s->clients,
            c_id
        );

        /* Already checked validity */
        assert(r);

        Packet* packet = enqueue(&r->msg_queue);
        assert(packet);
        (void)memcpy(packet, req, sizeof(Packet));
        packet->d.request.hdr.from = c->id;
        packet->d.request.hdr.t_id = t_id;
    }
}

static void send_transfer_response(
    Client* c, 
    const Packet* req, 
    const int type, 
    const Transfer_ID t_id
)
{
    assert(type == P_TRANSFER_INVALID || type == P_TRANSFER_VALID);

    Packet* const response = enqueue(&c->msg_queue);
    assert(response);

    /* Copy request */
    (void)memcpy(response, req, sizeof(Packet));

    PACKET_HDR(
        type,
        sizeof(response->d.request),
        response
    );

    if (type == P_TRANSFER_VALID)
        response->d.request.hdr.t_id = t_id;
}

static inline void debug_transfer(
    Server* s,
    const Packet* p, 
    const bool valid
)
{
    char buffer[124];
    size_t capacity = sizeof(buffer);
    char* buf_ptr = buffer;
    int n;

    n = snprintf(
        buf_ptr,
        capacity,
        valid ?
        "********** Transfer Request => VALID   **********\n" :
        "********** Transfer Request => INVALID **********\n"
    ); buf_ptr += n; capacity -= n;

    n = sprintf(
        buf_ptr,
        "\tFile:     %.*s\n"
        "\tSize:     %lu\n"
        "\tLocal ID: %lu\n"
        "\tUsers:\n",
        (int)sizeof(p->d.request.file_name), 
        p->d.request.file_name,
        p->d.request.file_size,
        p->d.request.client_transfer_id
    ); buf_ptr += n; capacity -= n;

    for (unsigned int i = 0; i < TRANSFER_CLIENTS_MAX; i++) {
        const Client_ID c_id = p->d.request.hdr.to[i];
        if (!c_id) break;

        n = snprintf(
            buf_ptr,
            capacity,
            "\t* %ld\n",
            c_id
        ); buf_ptr += n; capacity -= n;

        if (n < 0 || (size_t)n >= capacity)
            break;
    }

    SERVER_LOG(
        s,
        "%s*************************************************\n",
        buffer
    );
}

/* 
 * First validate if transfer is valid:
 * - Valid recipients
 * 
 * Struct d.request
 * - file_name[FILE_NAME_LEN]
 * - file_size
 * - client_transfer_id (client internal id till response)
 * 
 * Server Response:
 * P_TRANSFER_VALID or P_TRANSFER_INVALID
 * - Return transfer_request but possibly add transfer id
 */
static void packet_transfer(Server* s, Client* c)
{
    /* Validate all recipient clients */
    const Packet* const p = B_DATA(c->decrypt_buf);

    if (!db_transaction(&s->db, BEGIN_TRANSACTION)) {
        send_transfer_response(c, p, P_TRANSFER_INVALID, 0);
        return;
    }

    const Transfer_ID t_id = db_create_transfer(&s->db, c->id);

    if (!t_id || 
        !check_recipients(s, c, t_id, p) || 
        !db_transaction(&s->db, COMMIT_TRANSACTION)
    ) {
        goto error;
    }
    
    send_transfer_response(c, p, P_TRANSFER_VALID, t_id);
    echo_transfer_request(s, c, p, t_id);
    debug_transfer(s, p, true);
    return;

error:
    send_transfer_response(c, p, P_TRANSFER_INVALID, 0);
    db_transaction(&s->db, ROLLBACK_TRANSACTION);
    debug_transfer(s, p, false);
}

/* 
 * Recieve reply from client.
 * Try to update the TransferClient entry that matches:
 *  - transfer_id
 *  - client_id
 * If found update its 'accepted' field.
 * 
 * Then get the creator to try to echo back the message.
 */
static void packet_reply(Server* s, const Client* c)
{
    const Packet* const p = B_DATA(c->decrypt_buf);

    db_client_accept(
        &s->db, 
        c->id, 
        p->d.transfer_reply.hdr.t_id,
        p->d.transfer_reply.accept
    );

    const Client_ID c_id = db_get_creator_by_tid(
        &s->db, 
        p->d.transfer_reply.hdr.t_id
    );

    if (c_id != 0) {
        Client* creator = client_find_by_id(
            &s->clients, 
            c_id
        );
        assert(creator);

        Packet* response = enqueue(&creator->msg_queue);
        assert(response);
        
        (void)memcpy(response, p, sizeof(Packet));
        response->d.transfer_reply.hdr.from = c->id;    

        SERVER_LOG(
            s,
            "********** REPLY **********\n"
            "%s --- %s ---> %s\n"
            "***************************\n",
            c->name,
            p->d.transfer_reply.accept ? "ACCEPT" : "DENY",
            creator->name
        );
    }
}

/* 
 * Get all clients that accepted to the transfer.
 * Then attempt to echo back data message to client
 * if still active in transfer.
 */
static void packet_data(Server* s, const Client* c)
{
    const Packet* const p = B_DATA(c->decrypt_buf);

    Client_ID recp_id = db_get_client_all_accepted(
        &s->db,
        p->d.transfer_data.t_id
    );

    while (recp_id) {
        Client* other = client_find_by_id(&s->clients, recp_id);
        if (other) {
            Packet* packet = enqueue(&other->msg_queue);
            assert(packet);

            (void)memcpy(packet, p, sizeof(Packet));
        }
        recp_id = db_client_all_step_accepted(&s->db);
    }
}

static bool cancel_as_creator(
    Server* s, 
    const Client* c, 
    const Transfer_ID target
)
{
    Transfer_ID t_id = db_get_transfer(&s->db, c->id);
    
    while (t_id) {
        if (t_id == target) {
            LOG("CREATORR|\n");
            Client_ID t_client_id = db_get_client_all(
                &s->db, 
                t_id
            );

            while (t_client_id) {
                Client* recipient = client_find_by_id(
                    &s->clients, 
                    t_client_id
                );

                if (recipient) {
                    send_transfer_state(
                        recipient,
                        c->id,
                        t_id, 
                        P_TRANSFER_CANCEL
                    );
                }
                t_client_id = db_client_all_step(&s->db);
            }
            db_cleanup_transfer(&s->db, t_id);
            return true;
        }
        t_id = db_transfer_step(&s->db);
    }
    return false;
}

static bool cancel_as_recipient(
    Server* s, 
    const Client* c,
    const Transfer_ID target
)
{
    Transfer_Info info = db_get_creator(
        &s->db, 
        c->id
    );

    while (info.creator) {
        if (info.t_id == target) {
            Client* creator = client_find_by_id(
                &s->clients, 
                info.creator
            );

            if (creator) {
                send_transfer_state(
                    creator,
                    c->id,
                    info.t_id,
                    P_TRANSFER_CANCEL
                );
            }
            db_client_delete(&s->db, c->id);
            return true;
        }
        info = db_creator_step(&s->db);
    }
    return false;
}

/*
 * Recieve a Cancel from client can mean two things:
 * 1 - Host of transfer wants to cancel transfer
 * 2 - Recipient of transfer cancels recieving
 * 
 * If 1: cancel_as_creator
 * If 2: cancel_as_recipient
 */
static void packet_cancel(Server* s, Client* c)
{
    const Packet* const p = B_DATA(c->decrypt_buf);

    if (!db_transaction(&s->db, BEGIN_TRANSACTION))
        return;
    
    const Transfer_ID t_id = p->d.transfer_state.hdr.t_id;

    if (
        !cancel_as_creator(s, c, t_id) && 
        !cancel_as_recipient(s, c, t_id)
    ) {
        P_ERRORF(
            "Failed to CANCEL transfer '%ld'\n", 
            t_id
        );
    }

    (void)db_transaction(&s->db, COMMIT_TRANSACTION);
}

void analize_packet(Server* s, Client* c)
{
    const Packet* const p = (Packet*)B_DATA(c->decrypt_buf);
    switch (p->hdr.type)
    {
        case P_CLIENT_INTRO:
            packet_intro(s, c);
            break;

        case P_TRANSFER_REQUEST:
            packet_transfer(s, c);
            break;

        case P_TRANSFER_REPLY:
            packet_reply(s, c);
            break;

        case P_TRANSFER_DATA:
            packet_data(s, c);
            break;

        case P_TRANSFER_COMPLETE:
            LOG("Complete\n");
            // packet_complete(s, c);
            break;

        case P_TRANSFER_CANCEL:
            packet_cancel(s, c);
            break;
        
        default:
            P_ERROR("Unknown packet type\n");
            break;
    }
}

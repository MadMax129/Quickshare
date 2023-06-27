#include <string.h>
#include <assert.h>
#include "util.h"
#include "server.h"

static inline void server_resposne(Client* c, int type)
{
    Packet* packet = enqueue(&c->msg_queue);
    assert(packet);
    PACKET_HDR(type, 0, packet);
}

static void send_session_users(Server* s, Client* c)
{
    Packet* packet = NULL;
    uint8_t count = 0;

    for (unsigned int i = 0; i < MAX_CLIENTS; i++) {
        Client* user = &s->clients.list[i];

        if (user->id != c->id &&
            user->state == C_COMPLETE && 
            user->session_id == c->session_id) 
        {
            if (!packet) {
                packet = enqueue(&c->msg_queue);
                assert(packet);
                memset((void*)packet, 0, sizeof(Packet));
                PACKET_HDR(
                    P_SERVER_NEW_USERS,
                    sizeof(packet->d.users),
                    packet
                );
            }

            memcpy(
                &packet->d.users.names[count],
                user->name,
                PC_NAME_MAX_LEN
            );
            packet->d.users.ids[count] = user->id;
            ++packet->d.users.users_len;
            ++count;

            send_single_user(user, c, P_SERVER_NEW_USERS);
        }
    }
}

static void packet_intro(Server* s, Client* c)
{
    Packet* const p = c->p_buf;

    /* Make sure all data makes sense */
    if (c->state == C_COMPLETE                      ||
        p->d.intro.id_len >= (SESSION_ID_MAX_LEN-1) ||
        p->d.intro.name_len >= (PC_NAME_MAX_LEN-1)  ||
        p->d.intro.session > 1) 
    {
        /* Abnormal client behavior */
        server_resposne(c, P_SERVER_DENY);
        return;
    }

    p->d.intro.name[p->d.intro.name_len + 1] = '\0';
    p->d.intro.id[p->d.intro.id_len + 1]     = '\0';
   
    colored_printf(CL_BLUE,
        "Packet: Intro\n"
        "  Name:    '%.*s'\n"
        "  Session: %.*s\n"
        "  Type:    %s\n",
        p->d.intro.name_len, p->d.intro.name,
        p->d.intro.id_len, p->d.intro.id,
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

static bool check_recipients(Server* s, Client* c, 
                             const Transfer_ID t_id, const Packet* p)
{
    /* Check that all recipients are in session */
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

        printf("\t#%ld\n", c_id);
    }

    return true;
}

static void echo_transfer_request(Server* s, Client* c, 
                                  const Packet* req, const Transfer_ID t_id)
{
    for (int i = 0; i < TRANSFER_CLIENTS_MAX; i++) {
        Client_ID c_id = req->d.request.hdr.to[i];

        if (c_id == 0)
            break;

        Client* r = client_find_by_id(
            &s->clients,
            c_id
        );

        Packet* packet = enqueue(&r->msg_queue);
        assert(packet);
        (void)memcpy(packet, req, sizeof(Packet));
        packet->d.request.hdr.from = c->id;
        packet->d.request.hdr.t_id = t_id;
    }
}

static void packet_transfer(Server* s, Client* c)
{
    /* Validate all recipient clients */
    const Packet* const p = c->p_buf;

    colored_printf(CL_BLUE,
        "Transfer Request\n"
        "\t%.*s = %lu\n",   
        (int)sizeof(p->d.request.file_name), 
        p->d.request.file_name,
        p->d.request.file_size    
    );

    if (!db_transaction(&s->db, BEGIN_TRANSACTION)) {
        server_resposne(c, P_TRANSFER_INVALID);
        return;
    }

    const Transfer_ID t_id = db_create_transfer(&s->db, c->id);

    if (!t_id || 
        !check_recipients(s, c, t_id, p) || 
        !db_transaction(&s->db, COMMIT_TRANSACTION)
    ) {
        goto error;
    }
    
    Packet* response = enqueue(&c->msg_queue);
    assert(response);
    PACKET_HDR(
        P_TRANSFER_VALID, 
        sizeof(response->d.transfer_info), 
        response
    );
    response->d.transfer_info.id = t_id;

    echo_transfer_request(s, c, p, t_id);
    return;

error:
    server_resposne(c, P_TRANSFER_INVALID);
    db_transaction(&s->db, ROLLBACK_TRANSACTION);
}

static void packet_reply(Server* s, Client* c)
{
    const Packet* const p = c->p_buf;

    db_client_accept(
        &s->db, c->id, 
        p->d.transfer_reply.hdr.t_id,
        p->d.transfer_reply.accept
    );

    const Client_ID c_id = db_get_creator_by_tid(
        &s->db, 
        p->d.transfer_reply.hdr.t_id
    );

    if (c_id != 0) {
        Client* recp = client_find_by_id(&s->clients, c_id);
        assert(recp);
        Packet* response = enqueue(&recp->msg_queue);
        assert(response);
        (void)memcpy(response, p, sizeof(Packet));
        response->d.transfer_reply.hdr.from = c->id;    
    }
}

static void packet_data(Server* s, Client* c)
{
    const Packet* const p = c->p_buf;

    Client_ID recp_id = db_get_client_all_accepted(
        &s->db,
        p->d.transfer_data.hdr.t_id
    );

    while (recp_id) {
        Client* other = client_find_by_id(&s->clients, recp_id);
        if (other) {
            Packet* packet = enqueue(&other->msg_queue);
            assert(packet);
            memcpy(packet, p, sizeof(Packet));
        }
        recp_id = db_client_all_step_accepted(&s->db);
    }
}

void analize_packet(Server* s, Client* c)
{
    switch(c->p_buf->hdr.type)
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

        case P_TRANSFER_CANCEL:
        case P_TRANSFER_COMPLETE:
        default:
            P_ERROR("Unknown packet type\n");
            break;
    }
}
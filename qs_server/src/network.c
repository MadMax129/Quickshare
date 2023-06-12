#include <memory.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "server.h"
#include "util.h"

void create_socket(Server* s, const char* ip, const short port)
{
	s->sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (s->sock_fd < 0)
	    die("socket()");

	int enable = 1;
	if (setsockopt(s->sock_fd, SOL_SOCKET, SO_REUSEADDR, 
                    &enable, sizeof(enable)) < 0)
	    die("setsockopt(SO_REUSEADDR)");

	memset(&s->s_addr, 0, sizeof(s->s_addr));
	s->s_addr.sin_family = AF_INET;
	s->s_addr.sin_addr.s_addr = ip ? inet_addr(ip) : htonl(INADDR_ANY);
	s->s_addr.sin_port = htons(port);

	if (bind(s->sock_fd, 
            (struct sockaddr*)&s->s_addr, 
            sizeof(s->s_addr)) < 0)
	    die("bind()");

	if (listen(s->sock_fd, SOMAXCONN) < 0)
	    die("listen()");
}

void setup_poll(Server* s)
{
    s->epoll_fd = epoll_create1(0);

    if (s->epoll_fd == -1)
        die("epoll_create()");

    memset((void*)&s->event, 0, sizeof(s->event));
    s->event.events = EPOLLIN;
    s->event.data.fd = s->sock_fd;

    if (epoll_ctl(
        s->epoll_fd, 
        EPOLL_CTL_ADD,
        s->sock_fd, 
        &s->event
    ) == -1)
        die("epoll_ctl failed");

    s->events = calloc(MAX_EPOLL_EVENTS, sizeof(struct epoll_event));

    if (!s->events)
        die("Failed to alloc epoll events");
}

void server_free(Server* s)
{
    free(s->events);
}

static inline bool new_client_event(Server* s, int op, int fd)
{
    struct epoll_event ev;
    ev.events = EPOLLIN  | EPOLLRDHUP | 
                EPOLLHUP | EPOLLERR;
    ev.data.fd = fd;

	return epoll_ctl(s->epoll_fd, op, fd, &ev) != -1;
}

static void accept_client(Server* s)
{
    struct sockaddr_in addr;
    socklen_t socklen = sizeof(addr);
    Client* client = client_init(&s->clients);

    int conn = accept(
        s->sock_fd,
        (struct sockaddr*)&addr,
        &socklen
    );

    if (conn == -1 || !client) {
        printf("Client rejected %s:%d\n",
            inet_ntoa(addr.sin_addr), 
            addr.sin_port
        );
        close(conn);
        return;
    }

    // TODO ADD wait list/queue 
    /* 
        Once handshake has completed or
        no handshake has been initiated

        after timeout
        close connection
    */
    // SSL_set_fd(client->secure.ssl, conn);

    // if (SSL_accept(client->secure.ssl) <= 0) {
    //     ERR_print_errors_fp(stderr);
    // }

    assert(new_client_event(s, EPOLL_CTL_ADD, conn));

    client->fd = conn;
    memcpy((void*)&client->addr, (void*)&addr, sizeof(addr));

    printf("Client CONN [#%d, %d] %s:%d\n",
        (int)(client - s->clients.list), conn,
        inet_ntoa(addr.sin_addr), 
        addr.sin_port
    );
}

static void close_client(Server* s, int fd)
{
    Client* client = client_close(&s->clients, fd);

    if (!client) {
        printf("Error removing client\n");
        return;
    }

    printf("Client DISC [#%d, %d] %s:%d\n",
        (int)(client - s->clients.list), fd,
        inet_ntoa(client->addr.sin_addr), 
        client->addr.sin_port
    );

    assert(new_client_event(s, EPOLL_CTL_DEL, fd));


    /* Echo to session client disconnected */

    close(client->fd);
}

static Secure_State ssl_want_more(Client* c)
{
    Secure* s = &c->secure;
    char buf[64];
    int n;

    printf("More\n");

    do {
        n = BIO_read(
            s->w_bio, 
            buf, 
            sizeof(buf)
        );
        
        /* Queue SSL data to write */
        if (n > 0) {
            secure_queue_write(&c->secure, buf, n);
            printf("Encrypted %d\n", n);
        }
        else if (!BIO_should_retry(s->w_bio))
            return STATUS_FAIL;
    } while (n > 0);

    return STATUS_OK;
}

static Secure_State complete_handshake(Client* c)
{
    Secure_State status;

    int n = SSL_do_handshake(c->secure.ssl);
    
    printf("SSL Handshake State: %s\n", 
        SSL_state_string_long(c->secure.ssl)
    );

    if (n) c->state = C_SECURE;

    status = get_sslstate(&c->secure, n);

    if (status == STATUS_MORE_IO)
        return ssl_want_more(c);

    return status;
}

static inline void server_resposne(Client* c, int type)
{
    Packet* packet = enqueue(&c->msg_queue);
    assert(packet);
    packet->hdr.type = type;
}

static void send_single_user(Client* c, Client* c1)
{
    Packet* packet = enqueue(&c->msg_queue);
    memset((void*)packet, 0, sizeof(Packet));
    assert(packet);

    packet->hdr.type = P_SERVER_NEW_USERS;
    packet->d.users.users_len = 1;
    packet->d.users.ids[0] = c1->session_id;
    memcpy(
        &packet->d.users.names[0],
        c1->name,
        PC_NAME
    );
}

static void send_session_users(Server* s, Client* c)
{
    Packet* packet = NULL;
    uint8_t count = 0;

    for (unsigned int i = 0; i < MAX_CLIENTS; i++) {
        Client* user = &s->clients.list[i];

        if (user->id != c->id && user->session_id == c->session_id) {
            printf("Sending\n");
            if (!packet) {
                packet = enqueue(&c->msg_queue);
                assert(packet);
                memset((void*)packet, 0, sizeof(Packet));
                packet->hdr.type = P_SERVER_NEW_USERS;
            }

            memcpy(
                &packet->d.users.names[count],
                user->name,
                16
            );
            packet->d.users.ids[count] = user->session_id;
            ++packet->d.users.users_len;

            send_single_user(user, c);
        }
    }
}

static void packet_intro(Server* s, Client* c)
{
    Packet* const p = c->p_buf;

    /* Make sure all data makes sense */
    if (c->state == C_CONNECTED             ||
        p->d.intro.id_len >= (SESSION_ID-1) ||
        p->d.intro.name_len >= (PC_NAME-1)  ||
        p->d.intro.session > 1) 
    {
        /* Abnormal client behavior */
        server_resposne(c, P_SERVER_DENY);
        return;
    }

    p->d.intro.name[p->d.intro.name_len + 1] = '\0';
    p->d.intro.id[p->d.intro.id_len + 1]     = '\0';
   
    printf(
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
    c->state = C_CONNECTED;
    c->session_id = session;
    memcpy(c->name, p->d.intro.name, PC_NAME);

    send_session_users(s, c);
}

static void analize_packet(Server* s, Client* c)
{
    switch(c->p_buf->hdr.type)
    {
        case P_CLIENT_INTRO:
            packet_intro(s, c);
            break;

        default:
            printf("Unknown packet type\n");
            break;
    }
}

static bool decode_packet(Server* s, Packet* packet, ssize_t len, Client* c)
{
    char* packet_bytes = (char*)packet;
    int n = 0;

    assert(c);

    printf("Hee\n");

    while (len > 0) {
        n = BIO_write(c->secure.r_bio, (void*)packet_bytes, len);

        printf("BIO full write %d == %ld\n", n, len);

        if (n <= 0)
            return false;
        
        packet_bytes += n;
        len          -= n;

        /* Check if handshake has finished */
        if (!SSL_is_init_finished(c->secure.ssl)) {
            if (complete_handshake(c) == STATUS_FAIL)
                return false;
            /* Handshake not finished yet */
            if (!SSL_is_init_finished(c->secure.ssl))
                return true;
        }

        /* Begin decrypting data */
        do {
            n = SSL_read(
                c->secure.ssl, 
                ((char*)c->p_buf) + c->p_len, 
                sizeof(Packet)    - c->p_len
            );

            printf("SSL_RRRR =>%d\n", n);

            if (n > 0) {
                // printf("%d read = %.*s\n", n, n, ((char*)c->p_buf) + c->p_len);
                c->p_len += n;
                if (c->p_len > 3) {
                    (void)analize_packet;
                    printf("Read whole packet\n");
                    server_resposne(c, P_SERVER_OK);
                    // analize_packet(s, c);
                    c->p_len = 0;
                }
            }
            else {
                ERR_print_errors_fp(stderr);
            }
        } while (n > 0);

        Secure_State status = get_sslstate(&c->secure, n);

        if (status == STATUS_MORE_IO)
            return ssl_want_more(c) == STATUS_OK;
        else if (status == STATUS_FAIL)
            return false;
    }

    return true;
}

static void read_data(Server* s, int fd)
{
    static Packet packet;
    ssize_t n = read(fd, (void*)&packet, sizeof(Packet));

    if (n > 0) {
        if (decode_packet(
                s,
                &packet, 
                n, 
                client_find(&s->clients, fd)
            ))
            return;
    }

    /* Error reading data */
    
    printf("read()=%ld failed: %s\n", n, strerror(errno));
    close_client(s, fd);
}

static void check_for_write(Server* s)
{
    for (unsigned int i = 0; i < MAX_CLIENTS; i++) {
        if (s->clients.list[i].state == C_EMPTY)
            continue;

        struct epoll_event ev = {
            .events = EPOLLIN  | EPOLLRDHUP | 
                      EPOLLHUP | EPOLLERR,
            .data.fd = s->clients.list[i].fd
        };

        if (!queue_empty(&s->clients.list[i].msg_queue) ||
            s->clients.list[i].secure.e_len > 0)
            ev.events |= EPOLLOUT;

        epoll_ctl(
            s->epoll_fd, 
            EPOLL_CTL_MOD, 
            s->clients.list[i].fd, 
            &ev
        );
    }
}

static bool write_raw_data(int fd, char* data, size_t size)
{
    while (size > 0) {
        ssize_t n = write(
            fd, 
            data, 
            size
        );

        if (n > 0) {
            size -= n;
            data += n;
        }
        else {
            return false;
        }
    }

    return true;
}

static bool write_and_encrypt(Packet* p, Client* c)
{
    static Packet buf;
    size_t size = sizeof(Packet); 
    char* data  = (char*)p;
    Secure_State state;

    while (size > 0) {
        int n = SSL_write(c->secure.ssl, data, size);

        printf("SSL_write %d %ld %lu\n", n, sizeof(Packet), size);

        state = get_sslstate(&c->secure, n);

        if (n > 0) {
            data += n;
            size -= n;

            do {
                n = BIO_read(c->secure.w_bio, (void*)&buf, sizeof(Packet));
                printf("==>%d\n", n);
                if (n > 0) {
                    printf("Writing BIO %d\n", n);
                    secure_queue_write(&c->secure, (void*)&buf, n);
                }
                else if (!BIO_should_retry(c->secure.w_bio)) {
                    return false;
                }
            } while (n > 0);
        }

        printf("HEREEEE ==>%ld\n", size);
        if (state == STATUS_FAIL)
            return false;
    }

    return true;
}

static void write_data(Server* s, int fd)
{
    Client* c = client_find(&s->clients, fd);
    Queue*  q = &c->msg_queue;

    if (c->secure.e_len > 0) {
        if (!write_raw_data(fd, c->secure.e_buf, c->secure.e_len)) {
            printf("Failed to send openssl data\n");
            close_client(s, fd);
        }
        printf("Send\n");
        c->secure.e_len = 0;
    }

    if (!queue_empty(q)) {
        Packet* packet = dequeue(q);
        assert(packet);

        if (write_and_encrypt(packet, c)) {
            printf("Out and queue %d\n", q->count);
            return;
        }
        printf("Failed to write and encrypt\n");
        close_client(s, fd);
    }
}

void server_loop(Server* s)
{
    for (;;) {
        check_for_write(s);
        const int n_events = epoll_wait(
            s->epoll_fd, 
            s->events, 
            MAX_EPOLL_EVENTS, 
            -1
        );

        /* Failed */
        if (n_events == -1)
            break;

        for (int i = 0; i < n_events; i++) {
            const struct epoll_event* e = &s->events[i];
            /* Error */
            if (e->events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
                close_client(s, e->data.fd);

            /* Incoming connection */
            else if (e->data.fd == s->sock_fd)
                accept_client(s);

            /* Recv data */
            else if (e->events & EPOLLIN)
                read_data(s, e->data.fd);

            /* Write data */
            else if (e->events & EPOLLOUT)
                write_data(s, e->data.fd);
        }
    }

}
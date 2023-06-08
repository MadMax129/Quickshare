#include "server.h"
#include <memory.h>
#include <unistd.h>
#include <errno.h>

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
        return ssl_want_more(&c->secure);

    return status;
}

static bool decode_packet(Packet* packet, ssize_t len, Client* c)
{
    char* packet_bytes = (char*)packet;
    int n = 0;

    assert(c);

    while (len > 0) {
        n = BIO_write(c->secure.r_bio, (void*)packet_bytes, len);

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

            if (n > 0) {
                c->p_len += n;

                printf("read = %d\n", n);
            }
        } while (n > 0);

        Secure_State status = get_sslstate(&c->secure, n);

        if (status == STATUS_MORE_IO)
            return ssl_want_more(&c->secure) == STATUS_OK;
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
                &packet, 
                n, 
                client_find(&s->clients, fd)
            ))
            return;
    }

    /* Error reading data */
    
    printf("read() failed closing client: %s\n", strerror(errno));
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

        if (s->clients.list[i].secure.e_len > 0)
            ev.events |= EPOLLOUT;

        epoll_ctl(
            s->epoll_fd, 
            EPOLL_CTL_MOD, 
            s->clients.list[i].fd, 
            &ev
        );
    }
}

static void write_data(Server* s, int fd)
{
    Client* c = client_find(&s->clients, fd);

    ssize_t n = write(c->fd, c->secure.encrypted_buf, c->secure.e_len);
    if (n > 0) {
        printf("Writing %ld\n", n);
        if ((size_t)n < c->secure.e_len)
            memmove(
                c->secure.encrypted_buf, 
                c->secure.encrypted_buf + n, 
                c->secure.e_len-n
            );
        c->secure.e_len -= n;
        c->secure.encrypted_buf = (char*)realloc(c->secure.encrypted_buf, c->secure.e_len);
    }
    else {
        printf("Write error\n");
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
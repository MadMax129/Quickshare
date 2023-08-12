#include <memory.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include "util.h"
#include "server.h"
#include "die.h"
#include "mem.h"

extern volatile sig_atomic_t server_state;

static void set_nonblocking(const int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        P_ERROR("fcntl get flags\n");
        return;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        P_ERROR("fcntl()");
    }
}

void create_socket(Server* s, const char* ip, const short port)
{
	s->sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (s->sock_fd < 0)
	    die("socket()");

	int enable = 1;
	if (setsockopt(s->sock_fd, SOL_SOCKET, SO_REUSEADDR, 
                    &enable, sizeof(enable)) < 0)
	    die("setsockopt(SO_REUSEADDR)");

    set_nonblocking(s->sock_fd);

    memset(&s->s_addr, 0, sizeof(s->s_addr));
    s->s_addr.sin_family = AF_INET;
    s->s_addr.sin_addr.s_addr = ip ? 
        inet_addr(ip) : 
        htonl(INADDR_ANY);
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

    struct epoll_event event = {
        .events  = EPOLLIN,
        .data.fd = s->sock_fd
    };

    if (
        epoll_ctl(
            s->epoll_fd, 
            EPOLL_CTL_ADD,
            s->sock_fd, 
            &event) == -1
    )
        die("epoll_ctl failed");

    s->events = (struct epoll_event*)alloc(
        MAX_EPOLL_EVENTS * sizeof(struct epoll_event)
    );

    if (!s->events)
        die("Failed to alloc epoll events");
    
    memset(
        s->events, 
        0, 
        MAX_EPOLL_EVENTS * sizeof(struct epoll_event)
    );
}

bool new_client_event(Server* s, const int op, const int fd)
{
    struct epoll_event ev;
    ev.events = EPOLLIN  | EPOLLRDHUP | 
                EPOLLHUP | EPOLLERR;
    ev.data.fd = fd;

	return epoll_ctl(s->epoll_fd, op, fd, &ev) != -1;
}

void send_single_user(
    Client* recp, 
    const Client* c1, 
    const int type
)
{
    assert(
        type == P_SERVER_NEW_USERS ||
        type == P_SERVER_DEL_USERS
    );

    Packet* packet = enqueue(&recp->msg_queue);
    assert(packet);

    memset((void*)packet, 0, sizeof(Packet));
    
    PACKET_HDR(
        type, 
        type == P_SERVER_NEW_USERS ? 
            sizeof(packet->d.new_users) : 
            sizeof(packet->d.del_user), 
        packet
    );

    if (type == P_SERVER_NEW_USERS) {
        packet->d.new_users.users_len = 1;
        packet->d.new_users.ids[0]    = c1->id;
        memcpy(
            &packet->d.new_users.names[0],
            c1->name,
            PC_NAME_MAX_LEN
        );
    }
    else {
        packet->d.del_user.id = c1->id;
        memcpy(
            &packet->d.del_user.name,
            c1->name,
            PC_NAME_MAX_LEN
        );
    }
}

void send_transfer_state(
    Client* c, 
    const Client_ID from, 
    const Transfer_ID t_id, 
    const int type
)
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

static void accept_client(Server* s)
{
    for (;;) 
    {
        struct sockaddr_in addr;
        socklen_t socklen = sizeof(addr);

        const int conn = accept(
            s->sock_fd,
            (struct sockaddr*)&addr,
            &socklen
        );

        if (conn == -1) {
            if (
                errno == EAGAIN ||
                errno == EWOULDBLOCK
            ) {
                break;
            }
            else {
                return;
            }
        }

        Client* client = client_init(&s->clients);

        set_nonblocking(conn);

        if (
            !client || 
            !new_client_event(s, EPOLL_CTL_ADD, conn)
        ) {
            SERVER_LOG(
                s,
                "********** Client REJECTED **********\n"
                "\t%s:%d\n"
                "*************************************\n",
                inet_ntoa(addr.sin_addr), addr.sin_port
            );
            close_client(s, conn);
            return;
        }

        client->fd   = conn;
        client->addr = addr;
        SERVER_LOG(
            s,
            "********** Client ACCPTED **********\n"
            "\t#   %d\n"
            "\tFd: %d\n"
            "\t%s:%d\n"
            "*************************************\n",
            (int)(client - s->clients.list), conn,
            inet_ntoa(addr.sin_addr), addr.sin_port
        );
    }
}

static void check_for_write(const Server* s)
{
    for (unsigned int i = 0; i < MAX_CLIENTS; i++) {
        if (s->clients.list[i].state == C_EMPTY)
            continue;

        struct epoll_event ev = {
            .events = EPOLLIN  | EPOLLRDHUP | 
                      EPOLLHUP | EPOLLERR,
            .data.fd = s->clients.list[i].fd
        };

        if (
            !queue_empty(&s->clients.list[i].msg_queue) ||
            B_LEN(s->clients.list[i].secure.encrypted_buf) > 0
        ) {
            ev.events |= EPOLLOUT;
            LOGF("OUT %d\n", i);
        }

        epoll_ctl(
            s->epoll_fd, 
            EPOLL_CTL_MOD, 
            s->clients.list[i].fd, 
            &ev
        );
    }
}

void server_loop(Server* s)
{
    while (server_state == SERVER_OK) 
    {
        check_for_write(s);
        const int n_events = epoll_wait(
            s->epoll_fd, 
            s->events, 
            MAX_EPOLL_EVENTS, 
            -1
        );

        /* TODO: Check for client connected time vs. 
        send packet_intro kick client out if too long*/

        /* epoll_wait() Failed */
        if (n_events == -1) {
            P_ERROR("epoll_wait failed\n");
            break;
        }

        LOGF("===>EVE %d\n", n_events);

        for (int i = 0; i < n_events; i++) {
            const struct epoll_event* e = &s->events[i];

            /* Incoming connection */
            if (e->data.fd == s->sock_fd) {
                accept_client(s);
            }
            else {
                /* Error */
                if (e->events & (EPOLLERR | 
                                 EPOLLHUP | 
                                 EPOLLRDHUP)
                ) {
                    close_client(s, e->data.fd);
                    continue;
                }
                /* Recv data */
                if (e->events & EPOLLIN) {
                    LOG("DATA READ\n");
                    read_data(s, e->data.fd);
                }

                /* Write data */
                if (e->events & EPOLLOUT)
                    write_data(s, e->data.fd);
            }
        }
    }
}
#ifndef QS_SERVER
#define QS_SERVER

#include <stdbool.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include "client.h"
#include "secure.h"
#include "database.h"

#define MAX_EPOLL_EVENTS 64

typedef struct {
    int sock_fd, epoll_fd;
    struct epoll_event event, *events;
    struct sockaddr_in s_addr;
    Database db;
    Client_List clients;
} Server;

void create_socket(Server* s, const char* ip, const short port);
void setup_poll(Server* s);
void server_loop(Server* s);
void server_free(Server* s);

#endif /* QS_SERVER */
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

#define MAX_EPOLL_EVENTS (MAX_CLIENTS * 2)
#define LOG_FILE "../db/"

#define SERVER_LOG(server, str, ...) \
    do {                             \
        fprintf(                     \
            server->log_fd,          \
            str,                     \
            __VA_ARGS__              \
        );                           \
        fflush(server->log_fd);      \
    } while(0)
    
typedef enum {
    SERVER_OK,
    SERVER_CLOSE
} Server_State;

typedef struct {
    int sock_fd, epoll_fd;
    struct epoll_event* events;
    struct sockaddr_in s_addr;
    FILE* log_fd;
    Database db;
    Client_List clients;
} Server;

void create_socket(Server* s, const char* ip, const short port);
void setup_poll(Server* s);
void server_loop(Server* s);

bool new_client_event(Server* s, int op, int fd);
void send_single_user(
    Client* recp, 
    const Client* c1, 
    const int type
);
void send_transfer_state(
    Client* c, 
    const Client_ID from, 
    const Transfer_ID t_id, 
    const int type
);

void write_data(Server* s, int fd);
void read_data(Server* s, int fd);
void close_client(Server* s, int fd);
void analize_packet(Server* s, Client* c);

#endif /* QS_SERVER */
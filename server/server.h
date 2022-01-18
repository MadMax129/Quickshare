#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <unistd.h>

#define LOG(...) \
    do { \
        fprintf(stdout, "[ \033[33mLOG\033[0m ] " __VA_ARGS__); \
    } while (0)

#define ERROR(...) \
    do { \
        fprintf(stdout, "[ \033[31mERROR\033[0m ] " __VA_ARGS__); \
    } while (0)

/* Port from c++ 'networking.h' in client code */

#define USERNAME_MAX_LIMIT 16
#define PACKET_SIZE_MAX 512

enum Msg_Type {
    SERROR,
    NEW_CLIENT,
    CLIENT_ID,
    GLOBAL_CHAT,
    USER_ADD,
    USER_REMOVE,
};

typedef unsigned char Msg_Type;

struct Tcp_Msg {
    union {
        struct Id {
            unsigned char username[USERNAME_MAX_LIMIT];
        } id;

        struct Chat_Msg {
            unsigned char username[USERNAME_MAX_LIMIT];
            unsigned char data[PACKET_SIZE_MAX 
                                - USERNAME_MAX_LIMIT 
                                - sizeof(Msg_Type)];
        } msg;
    } data;
    Msg_Type m_type;
};

_Static_assert(sizeof(struct Tcp_Msg) == PACKET_SIZE_MAX, "Tcp_Msg is not equal to PACKET_SIZE");

/* Port End */

#define SERVER_PORT 5000
#define SERVER_IP "192.168.1.31"
#define LISTEN_QUEUE 30
#define MAX_CLIENTS 16

struct Client;

struct Server {
    int sockfd;
	struct sockaddr_in addr;
    unsigned int cli_amount;
    struct Client* client_list;
};

bool init_socket(const unsigned short port, const char* ip);
bool create_client_list();
void server_cleanup();
void server_loop();
const unsigned int find_empty_client();

enum Client_State {
    CS_EMPTY,
    CS_INIT,
    CS_ACTIVE
};

void* client_loop(void* arg);

struct Client {
    _Atomic int state;
    unsigned char usern[USERNAME_MAX_LIMIT];
	int tcp_connfd;
	struct Id id;
	pthread_t thread;
};

#endif /* SERVER_H */
#ifndef QUICKSHARE_CONFIG
#define QUICKSHARE_CONFIG

#include <time.h>

/* Collection of all Quickshare configurations */

/* Packet */
#define PACKET_MAX_SIZE 1440
#define PC_NAME_MAX_LEN 16
#define SESSION_ID_MAX_LEN 16
#define CLIENT_LIST_LEN 32
#define FILE_NAME_LEN 64
#define TRANSFER_CLIENTS_MAX 6
#define TRANSFER_QUEUE_MAX 6

/* File Manager */
#define FILE_BUFFER_SIZE (1024 * 3)

/* Mem Pool */
#define MEM_POOL_SIZE (1024 * 1024)

/* Network */
#define SERVER_MSG_QUEUE_SIZE 6

/* Transfers */
#define SIM_TRANSFERS_MAX 6

/* Threads */
#define THREAD_NUMBER_MAX 8

typedef time_t Client_ID;
typedef long Transfer_ID;

#if !defined(QS_SERVER_IP) && !defined(QS_SERVER_PORT)
#   define QS_SERVER_IP   "127.0.0.1"
#   define QS_SERVER_PORT 8080
#endif

#endif /* QUICKSHARE_CONFIG */
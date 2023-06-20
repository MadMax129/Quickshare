#ifndef QUICKSHARE_CONFIG
#define QUICKSHARE_CONFIG

#include <time.h>

/* Collection of all Quickshare configurations */
#define PACKET_MAX_SIZE 1440
#define PC_NAME_MAX_LEN 16
#define SESSION_ID_MAX_LEN 16
#define CLIENT_LIST_LEN 32
#define FILE_NAME_LEN 64
#define TRANSFER_CLIENTS_MAX 6

typedef time_t Client_ID;
typedef long Transfer_ID;

#endif /* QUICKSHARE_CONFIG */
#pragma once

/* Defines all configuration settings */

/********** allocation.hpp **********/
#define AMOUNT_OF_MSGS 3
#define AMOUNT_OF_BUFFERS 2
#define MEGABYTE (1024 * 1024)

/********** msg.hpp **********/
/* Defines the size of a packet send through the network. */
#define PACKET_MAX_SIZE 1304

/* Max file name */
#define MAX_FILE_NAME 64

/* Defines the max number of clients accepted */
#define MAX_CLIENTS 16

/* Max length for display computer host name */
#define CLIENT_NAME_LEN 16
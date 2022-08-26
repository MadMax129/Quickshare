#pragma once

/* Defines all configuration settings */

/********** allocation.hpp **********/
#define AMOUNT_OF_MSGS 3
#define AMOUNT_OF_BUFFERS 2
#define MEGABYTE (1024 * 1024)

/********** locator.hpp **********/
#define STATIC_QS_SERVER_IP "192.168.1.24"
#define STATIC_QS_SERVER_PORT 8543

/********** msg.hpp **********/
/* Defines the size of a packet send through the network. */
#define PACKET_MAX_SIZE 1304

/* Max file name */
#define MAX_FILE_NAME 64

/* Defines the max number of clients accepted */
#define MAX_CLIENTS 16

/* Max length for display computer host name */
#define CLIENT_NAME_LEN 16

/********** s_net.hpp **********/
/* Arbitrary server port that defines the central server location on local network */
#define STATIC_SERVER_PORT 8345
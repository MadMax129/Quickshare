#pragma once

/* Defines all configuration settings */

/********** allocation.hpp **********/
#define AMOUNT_OF_MSGS 3
#define AMOUNT_OF_BUFFERS 2
#define MEGABYTE (1024 * 1024)

/********** locator.hpp **********/
#define STATIC_QS_SERVER_IP "192.168.1.31"
#define STATIC_QS_SERVER_PORT 8080

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

/********** gui.hpp **********/
#define WINDOW_HEIGHT 500
#define WINDOW_WIDTH 300
#define ICON_PATH "../images/logo.png"
#define FONT_SIZE 15.0f

/********** main_menu.hpp **********/
#define MENU_BAR_MARGIN 20.0f
#define PATH_BUTTON_MARGIN 10.0f
#define REQUEST_MARGIN 20.0f
#define TWO_MENUS_Y_MARGIN 20.0f
#define MENUS_SIDE_MARGIN 16.0f
#define MENU_BOTTOM_MARGIN 35.0f
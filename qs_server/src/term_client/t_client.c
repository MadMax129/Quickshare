#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <ncurses.h>
#include <poll.h>
#include <assert.h>
#include <pthread.h>
#include "../../../shared/msg.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define WIDTH  80
#define HEIGHT 24

typedef struct Node {
    char name[PC_NAME_MAX_LEN];
    Client_ID id;
    struct Node* next;
} Node;

typedef struct LinkedList {
    Node* head;
} LinkedList;

typedef struct {
    int head, tail;
    struct {
        Transfer_ID t_id;
        Client_ID id;
        bool state;
    } transfer[8];
} Transfers;

static Transfers transfers;
static int sock;
static SSL_CTX* ctx;
static SSL* ssl;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static WINDOW *input_win, *output_win, *users_win, *output_subwin;
static LinkedList c_list;

void initialize_list(LinkedList* list) {
    list->head = NULL;
}

void add_node(LinkedList* list, const char* name, long id) {
    // Create a new node
    Node* new_node = (Node*)malloc(sizeof(Node));
    strcpy(new_node->name, name);
    new_node->id = id;
    new_node->next = NULL;

    // If the list is empty, set the new node as the head
    if (list->head == NULL) {
        list->head = new_node;
        return;
    }

    // Find the last node in the list
    Node* current = list->head;
    while (current->next != NULL) {
        current = current->next;
    }

    // Add the new node to the end of the list
    current->next = new_node;
}

void remove_node(LinkedList* list, const char* name) {
    // If the list is empty, return
    if (list->head == NULL) {
        return;
    }

    // If the head node matches the given name, remove it
    if (strcmp(list->head->name, name) == 0) {
        Node* temp = list->head;
        list->head = list->head->next;
        free(temp);
        return;
    }

    // Find the node with the given name
    Node* current = list->head;
    Node* previous = NULL;
    while (current != NULL && strcmp(current->name, name) != 0) {
        previous = current;
        current = current->next;
    }

    // If the node was found, remove it from the list
    if (current != NULL) {
        previous->next = current->next;
        free(current);
    }
}

void print_output(const char *format, ...);

void print_users() {
    werase(users_win);  // Clear the contents of the users_win window

    int user_line = 0;  // Keep track of current line in users_win
    int max_y = 0, max_x = 0;

    // Get the size of the window
    getmaxyx(users_win, max_y, max_x);

    Node* current = c_list.head;
    while (current != NULL) {
        // Check if we have space to print a new user
        if (user_line >= max_y - 1) {
            // Scroll window contents up by one line if there is no space
            wscrl(users_win, 1);
        } else {
            // Otherwise, increment the line counter
            user_line++;
        }
        mvwprintw(users_win, user_line, 1, "'%s' #%ld", current->name, current->id);
        current = current->next;
    }   

    // Draw the box and title again after scrolling
    box(users_win, 0, 0);
    mvwprintw(users_win, 0, 3, "[ USERS ]");

    // Refresh the window
    wrefresh(users_win);
}


int create_socket() {
    int sock;
    struct sockaddr_in addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        // perror("Unable to create socket");
        return 0;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    if(inet_pton(AF_INET, SERVER_IP, &addr.sin_addr)<=0) {
        // perror("Invalid address or address not supported");
       return 0;
    }

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        printw("Error connect\n"); 
        return 0;
    }

    return sock;
}

SSL_CTX* create_context() {
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    method = TLS_client_method();

    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

        SSL_CTX_set_max_proto_version(ctx, TLS1_3_VERSION);

    return ctx;
}

void print_output(const char *format, ...) {
    static int output_line = 0;  // Keep track of current line in output_subwin
    char str[1024];

    // Format the string
    va_list args;
    va_start(args, format);
    vsnprintf(str, 1024, format, args);
    va_end(args);

    int max_x = getmaxx(output_subwin);  // Get the width of the output_subwin window

    // Print the string to output_subwin with text wrapping
    mvwprintw(output_subwin, output_line, 0, "%s", str);

    // Increment the line counter based on the number of lines required for wrapping
    output_line += (strlen(str) + max_x - 1) / max_x;

    // Check if we need to scroll
    if (output_line >= HEIGHT / 2 - 3) {  // Adjusted for the box border and label
        wscrl(output_subwin, 1);  // Scroll subwindow contents up by one line
        output_line--;
    }

    // Refresh the windows
    wrefresh(output_win);
    wrefresh(output_subwin);
}


void print_packet(Packet* packet)
{
    switch (packet->hdr.type)
    {
        case P_SERVER_OK:
            print_output("Session OK");
            break;
        
        case P_SERVER_DENY:
            print_output("Session DENY");
            break;

        case P_TRANSFER_VALID:
            print_output(
                "Transfer valid t_id:%ld", 
                packet->d.transfer_info.id
            );

            transfers.transfer[transfers.head].t_id = packet->d.transfer_info.id;
            ++transfers.head;
            break;

        case P_TRANSFER_DATA:
            print_output("Data\n");
            break;

        case P_TRANSFER_REQUEST:
            print_output("Transfer Requested %s (c_id:%ld t_id:%ld)", 
                packet->d.request.file_name, packet->d.request.hdr.from,
                packet->d.request.hdr.t_id);
            break;

        case P_TRANSFER_REPLY:
            print_output("Transfer Reply %s (c_id:%ld t_id:%ld)", 
                packet->d.transfer_reply.accept ? "ACCEPT" : "DENY",
                packet->d.transfer_reply.hdr.from,
                packet->d.transfer_reply.hdr.t_id
            );
            for (int i = 0; i < 8; i++) {
                if (transfers.transfer[i].t_id == packet->d.transfer_reply.hdr.t_id) {
                    transfers.transfer[i].state = packet->d.transfer_reply.accept;
                    transfers.transfer[i].id = packet->d.transfer_reply.hdr.from;
                }
            }
            break;

        case P_TRANSFER_CANCEL:
            print_output(
                "Cancel Transfer (c_id:%ld t_id:%ld)", 
                packet->d.transfer_state.hdr.from, 
                packet->d.transfer_state.hdr.t_id
            );
            break;

        case P_TRANSFER_INVALID:
            print_output("Transfer invalid");
            break;
        
        case P_SERVER_NEW_USERS: {
            for (int i = 0 ; i < packet->d.new_users.users_len; i++) {
                add_node(&c_list, packet->d.new_users.names[i], packet->d.new_users.ids[i]);
            }
            print_users();
            break;
        }

        case P_SERVER_DEL_USERS: {
            // for (int i = 0 ; i < packet->d.de.users_len; i++) {
                remove_node(&c_list, packet->d.del_user.name);
            // }
            print_users();
            break;
        }
    }
}

void* read_thread(void* d)
{
    static Packet packet;
    /* Init */
    pthread_mutex_lock(&lock);
    ctx = create_context();
    sock = create_socket();

    if (sock == 0)
        return NULL;
    
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sock);

    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    PACKET_HDR(
        P_CLIENT_INTRO, 
        sizeof(packet.d.intro), 
        (&packet)
    );

    packet.d.intro.name_len = strlen((char*)d);
    strcpy(packet.d.intro.name, (char*)d);
    packet.d.intro.id_len = 1;
    strcpy(packet.d.intro.id, "t");
    packet.d.intro.session = 0;

    assert(
        SSL_write(
            ssl, 
            (void*)&packet, 
            sizeof(Packet_Hdr) + sizeof(packet.d.intro)
        ) == sizeof(Packet_Hdr) + sizeof(packet.d.intro)
    );

    pthread_mutex_unlock(&lock);

    struct pollfd fds;
    fds.fd = sock;
    fds.events = POLLIN;

    for (;;) {
        int ret = poll(&fds, 1, -1); 

        if (ret > 0) {
            // Check if data is available to read
            if (fds.revents & POLLIN) 
            {
                pthread_mutex_lock(&lock);
                int n = SSL_read(ssl, (void*)&packet, sizeof(Packet_Hdr));

                if (packet.hdr.size > 0) {
                    n += SSL_read(ssl, (char*)&packet + n, packet.hdr.size);
                }

                if (n <= 0) {
                    int error = SSL_get_error(ssl, n);
                    if (error ==  SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE) {
                        printf("*******Trying again\n");
                        continue;
                    }
                    else if (error == SSL_ERROR_ZERO_RETURN) {
                        return NULL;
                    }
                    else {
                        ERR_print_errors_fp(stderr);
                        return NULL;
                    }
                }

                assert(n == sizeof(Packet_Hdr) + packet.hdr.size);

                pthread_mutex_unlock(&lock);
                
                print_packet(&packet);
            }
        } else {
            return NULL;
        }

    }
}

void transfer_req(Packet* packet, long c_id)
{
    PACKET_HDR(
        P_TRANSFER_REQUEST,
        sizeof(packet->d.request),
        packet
    );

    strcpy(packet->d.request.file_name, "test.txt");
    packet->d.request.file_size = 120;
    packet->d.request.hdr.to[0] = c_id;
    packet->d.request.hdr.to[1] = 0;
    packet->d.request.hdr.to[2] = 0;
    assert(
        SSL_write(
            ssl, 
            (void*)packet, 
            sizeof(Packet_Hdr) + sizeof(packet->d.request)
        ) == sizeof(Packet_Hdr) + sizeof(packet->d.request)
    );
}

void reply(Packet* packet, long t_id, bool response)
{
    PACKET_HDR(
        P_TRANSFER_REPLY,
        sizeof(packet->d.transfer_reply),
        packet
    );

    packet->d.transfer_reply.hdr.t_id = t_id;
    packet->d.transfer_reply.accept = response;
    assert(
        SSL_write(
            ssl, 
            (void*)packet, 
            sizeof(Packet_Hdr) + sizeof(packet->d.transfer_reply)
        ) == sizeof(Packet_Hdr) + sizeof(packet->d.transfer_reply)
    );
}

void send_data(Packet* packet, long t_id)
{
    for (int i = 0; i < 8; i++) {
        if (transfers.transfer[i].t_id == t_id && 
            !transfers.transfer[i].state) {
            print_output("cannot send. No client response");
            return;
        }
    }

    PACKET_HDR(
        P_TRANSFER_DATA,
        sizeof(packet->d.transfer_data),
        packet
    );

    packet->d.transfer_data.hdr.t_id = t_id;
    packet->d.transfer_data.b_size = 10;
    assert(
        SSL_write(
            ssl, 
            (void*)packet, 
            sizeof(Packet_Hdr) + sizeof(packet->d.transfer_data)
        ) == sizeof(Packet_Hdr) + sizeof(packet->d.transfer_data)
    );
}

void* input_thread(void* d)
{
    static Packet packet;
    char input[256];

    while(1) {
        werase(input_win);
        box(input_win, 0, 0);
        mvwprintw(input_win, 0, 3, "[ INPUT WINDOW ]");
        wrefresh(input_win);
        echo();
        mvwgetnstr(input_win, 1, 1, input, 255);
        noecho();
        
        char command = input[0];
        long value = atol(input + 2);

        pthread_mutex_lock(&lock);
        if (command == 't') {
            transfer_req(&packet, value);
        } 
        else if (command == 'y' || command == 'n') {
            reply(
                &packet,
                value,
                command == 'y'
            );
        }
        else if (command == 'd') {
            send_data(&packet, value);
        }
        else {
            print_output("Invalid command received.");
        }
        pthread_mutex_unlock(&lock);
    }
}

int main(int argc, const char** argv) 
{
    if (argc != 2) {
        printf("usage: quickshare <username>\n");
        exit(0);
    }

    /* Initialize the OpenSSL library */
    SSL_library_init();
    initscr();

    // Create windows for input, output, and users
    input_win = newwin(HEIGHT/2, WIDTH/2, 0, 0);
    output_win = newwin(HEIGHT/2, WIDTH/2, HEIGHT/2, 0);
    users_win = newwin(HEIGHT, WIDTH/2, 0, WIDTH/2);
    output_subwin = subwin(output_win, HEIGHT / 2 - 2, WIDTH / 2 - 2, HEIGHT / 2 + 1, 1);
    scrollok(output_subwin, TRUE);
    initialize_list(&c_list);

    box(input_win, 0, 0);
    box(output_win, 0, 0);
    box(users_win, 0, 0);

    mvwprintw(input_win, 0, 3, "[ INPUT WINDOW ]");
    mvwprintw(output_win, 0, 3, "[ OUTPUT WINDOW ]");
    mvwprintw(users_win, 0, 3, "[ USERS ]");

    wrefresh(input_win);
    wrefresh(output_win);
    wrefresh(users_win);
    // wrefresh(pad);

    pthread_t r,i;
    pthread_create(&r, NULL, read_thread, (void*)argv[1]);
    pthread_create(&i, NULL, input_thread, NULL);
    pthread_join(r, NULL);

    /* Cleanup */
    delwin(input_win);
    delwin(output_win);
    delwin(users_win);
    endwin();

    SSL_free(ssl);
    close(sock);
    SSL_CTX_free(ctx);
    return 0;
}
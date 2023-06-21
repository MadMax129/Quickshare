#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "../../include/msg.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

int create_socket() {
    int sock;
    struct sockaddr_in addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Unable to create socket");
        exit(EXIT_FAILURE);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    if(inet_pton(AF_INET, SERVER_IP, &addr.sin_addr)<=0) {
        perror("Invalid address or address not supported");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Unable to connect to server");
        exit(EXIT_FAILURE);
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

int main(int argc, const char** argv) {
    SSL_CTX* ctx;
    SSL* ssl;
    int sock;

    /* Initialize the OpenSSL library */
    SSL_library_init();

    ctx = create_context();

    sock = create_socket();

    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sock);

    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    /* Send large array of data */
    Packet packet = {
        .hdr.type = P_CLIENT_INTRO
    };

    packet.hdr.size = sizeof(packet.d.intro);


    packet.d.intro.name_len = strlen(argv[1]);
    strcpy(packet.d.intro.name, argv[1]);
    packet.d.intro.id_len = 1;
    strcpy(packet.d.intro.id, "t");
    packet.d.intro.session = 0;

    int n = SSL_write(ssl, (void*)&packet, sizeof(Packet_Hdr) + sizeof(packet.d.intro));

    //gcc test_client.c -o t1 -lcrypto -lssl -I"../../../shared"
    for (;;) {
        n = SSL_read(ssl, (void*)&packet, sizeof(Packet_Hdr));

        if (packet.hdr.size > 0) {
            printf("Read more\n");
            n = SSL_read(ssl, (char*)&packet + n, packet.hdr.size);
        }

        if (n <= 0) {
            int error = SSL_get_error(ssl, n);
            if (error ==  SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE) {
                printf("Trying again\n");
                continue;
            }
            else if (error == SSL_ERROR_ZERO_RETURN) {
                exit(1);
            }
            ERR_print_errors_fp(stderr);
            exit(1);
        }

        switch (packet.hdr.type)
        {
            case P_SERVER_OK:
                printf("Session OK\n");
                break;
            
            case P_SERVER_DENY:
                printf("Session DENY\n");
                break;

            case P_TRANSFER_VALID:
                printf("Transfer valid %ld\n", packet.d.transfer_info.id);
                break;

            case P_TRANSFER_CANCEL:
                printf("Cancel transcation %ld %ld\n", 
                packet.d.transfer_state.hdr.from, packet.d.transfer_state.hdr.t_id);
                break;

            case P_TRANSFER_INVALID:
                printf("Transfer invalid\n");
                break;
            
            case P_SERVER_NEW_USERS:
                printf("New users\n");
                Client_ID id;
                for (int i = 0 ; i < packet.d.users.users_len; i++) {
                    printf(
                        "'%s' #%ld\n",
                        packet.d.users.names[i],
                        packet.d.users.ids[i]
                    );
                    id = packet.d.users.ids[i];
                }

                packet.hdr.type = P_TRANSFER_REQUEST;
                packet.hdr.size = sizeof(packet.d.request);
                strcpy(packet.d.request.file_name, "test.txt");
                packet.d.request.file_size = 120;
                packet.d.request.hdr.to[0] = id;
                packet.d.request.hdr.to[1] = 0;
                // n = SSL_write(ssl, (void*)&packet, sizeof(Packet_Hdr) + sizeof(packet.d.request));
                break;

            case P_SERVER_DEL_USERS:
                printf("Users disconnected\n");
                for (int i = 0 ; i < packet.d.users.users_len; i++) {
                    printf(
                        "'%s' #%ld\n",
                        packet.d.users.names[i],
                        packet.d.users.ids[i]
                    );
                    id = packet.d.users.ids[i];
                }
                break;
        }
    }

    // getc(stdin);
    // if (SSL_shutdown(ssl) == 0)
    // {
    //     /* Wait for the peer's close notify */
    //     SSL_shutdown(ssl);
    // }

    /* Cleanup */
    SSL_free(ssl);
    close(sock);
    SSL_CTX_free(ctx);
    return 0;
}

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "server.h"

static Secure_State ssl_want_more(Client* c)
{
    Secure* s = &c->secure;
    char buf[64];
    int n;

    do {
        n = BIO_read(
            s->w_bio, 
            buf, 
            sizeof(buf)
        );
        
        /* Queue SSL data to write */
        if (n > 0)
            secure_queue_write(&c->secure, buf, n);
        else if (!BIO_should_retry(s->w_bio))
            return STATUS_FAIL;
    } while (n > 0);

    return STATUS_OK;
}

static Secure_State complete_handshake(Client* c)
{
    Secure_State status;

    int n = SSL_do_handshake(c->secure.ssl);
    
    printf("SSL Handshake State: %s\n", 
        SSL_state_string_long(c->secure.ssl)
    );

    if (n) c->state = C_SECURE;

    status = get_sslstate(&c->secure, n);

    if (status == STATUS_MORE_IO)
        return ssl_want_more(c);

    return status;
}

static int decrypt_data(Server* s, Client* c)
{
    int n;
    do {
        n = SSL_read(
            c->secure.ssl, 
            ((char*)c->p_buf) + c->p_len, 
            (sizeof(Packet_Hdr) + c->p_size) - c->p_len
        );

        if (n > 0) {
            c->p_len += n;
            if (c->p_len == sizeof(Packet_Hdr)) {
                /* Read size of packet */
                Packet_Hdr* hdr = (Packet_Hdr*)c->p_buf;
                c->p_size = hdr->size;
            }

            if (c->p_len == (c->p_size + sizeof(Packet_Hdr))) {
                analize_packet(s, c);
                c->p_len  = 0;
                c->p_size = 0;
            }
        }
        else {
            ERR_print_errors_fp(stderr);
        }
    } while (n > 0);

    return n;
}

static bool decode_packet(Server* s, Packet* packet, ssize_t len, Client* c)
{
    char* packet_bytes = (char*)packet;
    int n = 0;

    assert(c);

    while (len > 0) {
        n = BIO_write(c->secure.r_bio, (void*)packet_bytes, len);

        printf("BIO full write %d == %ld\n", n, len);

        if (n <= 0)
            return false;
        
        packet_bytes += n;
        len          -= n;

        /* Check if handshake has finished */
        if (!SSL_is_init_finished(c->secure.ssl)) {
            if (complete_handshake(c) == STATUS_FAIL)
                return false;
            /* Handshake not finished yet */
            if (!SSL_is_init_finished(c->secure.ssl))
                return true;
        }

        /* Begin decrypting data */
        n = decrypt_data(s, c);

        Secure_State status = get_sslstate(&c->secure, n);

        if (status == STATUS_MORE_IO)
            return ssl_want_more(c) == STATUS_OK;
        else if (status == STATUS_FAIL)
            return false;
    }

    return true;
}

void read_data(Server* s, int fd)
{
    static Packet packet;
    ssize_t n = read(fd, (void*)&packet, sizeof(Packet));

    if (n > 0) {
        if (decode_packet(
                s,
                &packet, 
                n, 
                client_find(&s->clients, fd)
            ))
            return;
    }

    /* Error reading data */
    
    printf("read()=%ld failed: %s\n", n, strerror(errno));
    close_client(s, fd);
}
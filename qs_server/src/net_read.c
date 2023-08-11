#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "server.h"
#include "util.h"

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

    if (n == 1) 
        c->state = C_SECURE;

    status = get_sslstate(&c->secure, n);

    if (status == STATUS_MORE_IO)
        return ssl_want_more(c);

    return status;
}

static int decrypt_data(Server* s, Client* c)
{
    Buffer* buf = &c->decrypt_buf;
    int n = 0;

    do {
        n = SSL_read(
            c->secure.ssl, 
            (char*)buf->data    + buf->len, 
            (sizeof(Packet_Hdr) + buf->size) - buf->len
        );

        if (n > 0) {
            buf->len += n;
            
            /* Read size of packet */
            if (buf->len == sizeof(Packet_Hdr))
                buf->size = ((Packet_Hdr*)buf->data)->size;

            if (buf->len == (buf->size + sizeof(Packet_Hdr))) {
                analize_packet(s, c);
                buf->len  = 0;
                buf->size = 0;
            }
        }
    } while (n > 0);

    return n;
}

static bool decode_packet(Server* s, Packet* packet, size_t len, Client* c)
{
    const char* packet_bytes = (char*)packet;
    int n = 0;

    while (len > 0) 
    {
        n = BIO_write(c->secure.r_bio, packet_bytes, len);

        // printf("BIO full write %d == %ld\n", n, len);

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
    Client* const client = client_find(
        &s->clients,
        fd
    );
    assert(client);
    Buffer* const buf = &client->read_buf;

    const ssize_t n = read(
        client->fd, 
        (char*)buf->data,
        sizeof(Packet) / 4
    );

    if (n > 0) {
        if (!decode_packet(s, (Packet*)buf->data, n, client)) {
            printf("decode_packet failed:\n");
            ERR_print_errors_fp(stdout);
            close_client(s, client->fd);
        }
    }
    else if (n == -1) {
        if (errno == EAGAIN ||
            errno == EWOULDBLOCK)
            return;
    }
    else {
        close_client(s, fd);
    }
}
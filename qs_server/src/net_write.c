#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "server.h"
#include "util.h"

typedef enum {
    WRITE_ERROR,
    WRITE_OK,
    WRITE_BLOCK
} Write_Result;

static Write_Result write_bytes(Client* c)
{
    Buffer* const buf = &c->secure.encrypted_buf;

    while (buf->len > 0)
    {
        ssize_t n = write(
            c->fd,
            buf->data,
            buf->len
        );

        if (n == -1          && 
            (errno == EAGAIN || 
             errno == EWOULDBLOCK)
        ) {
            return WRITE_BLOCK;
        }
        else if (n > 0) {
            if ((buf->len - n) != 0) {
                (void*)memmove(
                    buf->data,
                    (char*)buf->data + n,
                    buf->len         - n
                );
            }
            buf->len -= n;
        }
        else {
            return WRITE_ERROR;
        }
    }

    return WRITE_OK;
}

static bool encrypt(Packet* p, Client* c)
{
    char buf[64];
    size_t size = sizeof(Packet_Hdr) + p->hdr.size; 
    const char* data  = (const char*)p;
    Secure_State state;

    while (size > 0) {
        int n = SSL_write(c->secure.ssl, data, size);

        printf("SSL_write %d %ld %lu\n", n, sizeof(Packet), size);

        state = get_sslstate(&c->secure, n);

        assert(state != STATUS_MORE_IO);

        if (n > 0) {
            data += n;
            size -= n;

            do {
                n = BIO_read(c->secure.w_bio, &buf, sizeof(buf));
                if (n > 0) 
                    secure_queue_write(&c->secure, (const void*)&buf, n);
                else if (!BIO_should_retry(c->secure.w_bio))
                    return false;
            } while (n > 0);
        }

        if (state == STATUS_FAIL)
            return false;
    }

    return true;
}

void write_data(Server* s, int fd)
{
    Client* const client = client_find(&s->clients, fd);
    assert(client);

    Queue* const q = &client->msg_queue;

    Write_Result w_state = WRITE_OK;

    // TODO: RETHINK LOOP
    do {
        /* Write encrypted bytes */
        if (B_LEN(client->secure.encrypted_buf) > 0) {
            w_state = write_bytes(client);
            if (w_state == WRITE_ERROR) {
                P_ERROR("Failed to write data\n");
                close_client(s, fd);
            }
        }

        if (!queue_empty(q)) {
            Packet* packet = dequeue(q);
            assert(packet);

            if (!encrypt(packet, client)) {
                printf("Failed to write and encrypt\n");
                close_client(s, fd);
            }
        }
    } while (!queue_empty(q) && w_state != WRITE_BLOCK);
}
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "server.h"

static bool write_raw_data(int fd, char* data, size_t size)
{
    while (size > 0) {
        ssize_t n = write(
            fd, 
            data, 
            size
        );

        if (n > 0) {
            size -= n;
            data += n;
        }
        else {
            return false;
        }
    }

    return true;
}

static bool write_and_encrypt(Packet* p, Client* c)
{
    static Packet buf;
    size_t size = sizeof(Packet_Hdr) + p->hdr.size; 
    char* data  = (char*)p;
    Secure_State state;

    while (size > 0) {
        int n = SSL_write(c->secure.ssl, data, size);

        printf("SSL_write %d %ld %lu\n", n, sizeof(Packet), size);

        state = get_sslstate(&c->secure, n);

        if (n > 0) {
            data += n;
            size -= n;

            do {
                n = BIO_read(c->secure.w_bio, (void*)&buf, sizeof(Packet));
                if (n > 0) 
                    secure_queue_write(&c->secure, (void*)&buf, n);
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
    Client* c = client_find(&s->clients, fd);
    Queue*  q = &c->msg_queue;

    if (c->secure.e_len > 0) {
        if (!write_raw_data(fd, c->secure.e_buf, c->secure.e_len)) {
            printf("Failed to send openssl data\n");
            close_client(s, fd);
        }
        c->secure.e_len = 0;
    }

    if (!queue_empty(q)) {
        Packet* packet = dequeue(q);
        assert(packet);

        if (write_and_encrypt(packet, c))
            return;
        printf("Failed to write and encrypt\n");
        close_client(s, fd);
    }
}
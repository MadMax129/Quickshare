#include "secure.h"
#include "server.h"
#include <memory.h>
#include "util.h"

void ssl_init(const char * certfile, const char* keyfile)
{
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ERR_load_crypto_strings();

    ssl_ctx = SSL_CTX_new(TLS_method());
    if (!ssl_ctx)
        die("SSL_CTX_new()");

    /* Load certificate and private key files, and check consistency */
    if (certfile && keyfile) {
        if (SSL_CTX_use_certificate_file(ssl_ctx, certfile,  SSL_FILETYPE_PEM) != 1)
            die("SSL_CTX_use_certificate_file failed");

        if (SSL_CTX_use_PrivateKey_file(ssl_ctx, keyfile, SSL_FILETYPE_PEM) != 1)
            die("SSL_CTX_use_PrivateKey_file failed");

        /* Make sure the key and certificate file match. */
        if (SSL_CTX_check_private_key(ssl_ctx) != 1)
            die("SSL_CTX_check_private_key failed");
    
        printf("certificate and private key loaded and verified\n");
    }

    /* Recommended security options */
    SSL_CTX_set_options(
        ssl_ctx, 
        SSL_OP_ALL        | 
        SSL_OP_NO_SSLv2   | 
        SSL_OP_NO_SSLv3   |
        SSL_OP_NO_TLSv1   |
        SSL_OP_NO_TLSv1_1 |
        SSL_OP_NO_TLSv1_2
    );
}

void secure_queue_write(Secure* s, const char *buf, size_t len)
{
    s->e_buf = (char*)realloc(s->e_buf, s->e_len + len);
    memcpy(s->e_buf + s->e_len, buf, len);
    s->e_len += len;
}

Secure_State get_sslstate(Secure* s, int ret)
{
    switch (SSL_get_error(s->ssl, ret)) 
    {
        case SSL_ERROR_NONE:
            return STATUS_OK;
        
        case SSL_ERROR_WANT_WRITE:
        case SSL_ERROR_WANT_READ:
            return STATUS_MORE_IO;

        case SSL_ERROR_ZERO_RETURN:
        case SSL_ERROR_SYSCALL:
        default:
            return STATUS_FAIL;
    }
}

void secure_init(Secure* s)
{
    s->r_bio = BIO_new(BIO_s_mem());
    s->w_bio = BIO_new(BIO_s_mem());
    s->ssl = SSL_new(ssl_ctx);

    SSL_set_accept_state(s->ssl);

    SSL_set_bio(s->ssl, s->r_bio, s->w_bio);
}

void secure_free(Secure* s)
{
    if (SSL_shutdown(s->ssl) == 0)
        SSL_shutdown(s->ssl);
    SSL_free(s->ssl);

    if (s->e_buf)
        memset(s->e_buf, 0, s->e_len);
    s->e_len = 0;
}
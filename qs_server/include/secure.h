#ifndef QS_SECURE
#define QS_SECURE

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>

typedef struct {
    SSL* ssl;
    BIO* r_bio; 
    BIO* w_bio;
    char* encrypted_buf;
    unsigned int e_len;
} Secure;

typedef enum {
    STATUS_OK,
    STATUS_MORE_IO,
    STATUS_FAIL
} Secure_State;

extern SSL_CTX* ssl_ctx;

void ssl_init(const char* certfile, const char* keyfile);
void secure_init(Secure* s);
void secure_free(Secure* s);
void secure_queue_write(Secure* s, const char *buf, size_t len);
Secure_State get_sslstate(Secure* s, int ret);
Secure_State ssl_want_more(Secure* c);
void ssl_free();

#endif /* QS_SECURE */
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
} Secure;

extern SSL_CTX* ssl_ctx;

void ssl_init(const char* certfile, const char* keyfile);
void secure_init(Secure* s);
void secure_free(Secure* s);
void ssl_free();

#endif /* QS_SECURE */
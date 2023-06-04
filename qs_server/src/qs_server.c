#include <stdio.h>
#include <stdarg.h>
#include "server.h"
#include "client.h"

SSL_CTX* ssl_ctx;

_Noreturn void die(const char* format, ...)
{
	va_list vargs;
    va_start (vargs, format);
	fprintf(stderr, CL_RED "[ ERROR ] ");
    vfprintf (stderr, format, vargs);
	fprintf(stderr, CL_RESET);
    fprintf (stderr, "\n");
    va_end (vargs);
    exit(EXIT_FAILURE);
}

int main(const int argc, const char* argv[]) 
{
	(void)argc;
	(void)argv;

	Server server;

	ssl_init("./cert/server.crt", "./cert/server.key");

	client_list_init(&server.clients);
	create_socket(&server, NULL, 8080);
	setup_poll(&server);

	server_loop(&server);

	/* Free Resources */
	client_list_free(&server.clients);
}
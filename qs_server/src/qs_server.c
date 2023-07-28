/* @file qs_server.c
 * 
 * @brief Quickshare server main file
 *      
 * Copyright (c) 2023 Maks S
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */ 

#include <stdio.h>
#include <stdarg.h>
#include <signal.h>

#include "util.h"
#include "server.h"
#include "client.h"
#include "database.h"
#include "mem.h"

volatile sig_atomic_t server_state = SERVER_OK;

_Noreturn void die(const char* format, ...)
{
	va_list vargs;
    va_start(vargs, format);
	fprintf(stderr, "\x1b[31m" "[ ERROR ] ");
    vfprintf(stderr, format, vargs);
	fprintf(stderr, "\x1b[0m");
    fprintf(stderr, "\n");
    va_end(vargs);
    exit(EXIT_FAILURE);
}

static void setup_log(Server* server)
{
	FILE* file_fd = NULL;
    char file_name[64] = {0};

    time_t raw_time;
    struct tm* time_info;
    time(&raw_time);
    time_info = localtime(&raw_time);

    strftime(
		file_name, 
		sizeof(file_name), 
		LOG_FILE "%Y-%m-%d-server-log.txt", 
		time_info
	);

    if ((file_fd = fopen(file_name, "r")) != NULL) {
        fclose(file_fd);
        server->log_fd = fopen(file_name, "a");
    } else {
        server->log_fd = fopen(file_name, "w");
        if (server->log_fd == NULL)
			die("Can't create log %s\n", file_name);
    }
}

static void close_log(Server* server)
{
	fclose(server->log_fd);
}

static void sigint_handler(int signal) 
{
	(void)signal;
	server_state = SERVER_CLOSE;
}

static void setup_inter()
{
	struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1)
		die("Failed to setup interrupt");
}

int main(const int argc, const char* argv[]) 
{
	(void)argc;
	(void)argv;

	Server server;

	setup_inter();
	setup_log(&server);
	db_init(&server.db);
	ssl_init("./cert/server.crt", "./cert/server.key");
	if (!mem_pool_init(MEM_POOL_SIZE))
		die("Memory pool fail");
	client_list_init(&server.clients);

	create_socket(&server, NULL, 8080);
	setup_poll(&server);

	server_loop(&server);

	/* Free Resources */
	close_log(&server);
	client_list_free(&server.clients);
	mem_pool_free();
}
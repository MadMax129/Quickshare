#include "server.h"
#include <assert.h>
#include <arpa/inet.h>

static struct Server server;

bool init_socket(const unsigned short port, const char* ip)
{
	if ((server.sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		ERROR("Socket init faliure\n");
		return false;
	}

	server.addr.sin_family = AF_INET;
	server.addr.sin_addr.s_addr = inet_addr(ip);
	server.addr.sin_port = htons(port);

	if ((bind(server.sockfd, (struct sockaddr*)&server.addr,
			  sizeof(struct sockaddr))) != 0) {
		ERROR("Bind failed\n");
		return false;
	}

	if (listen(server.sockfd, LISTEN_QUEUE)) {
		ERROR("Listen failed\n");
		return false;
	}

	return true;
}

bool create_client_list()
{
	server.client_list = (struct Client*)
		malloc(sizeof(struct Client)*MAX_CLIENTS);
	
	if (!server.client_list) {
		ERROR("Malloc failure client_list\n");
		return false;
	}

	memset(server.client_list, 0, sizeof(struct Client)*MAX_CLIENTS);

	return true;
}

const unsigned int find_empty_client()
{
	for (unsigned int i = 0; i < MAX_CLIENTS; i++) {
		if (atomic_load(&server.client_list[i].state) == CS_EMPTY) {
			atomic_store(&server.client_list[i].state, CS_INIT);
			return i;
		}
	}
	assert(false);
}

void* client_loop(void* arg)
{
	struct Client *const cli = (struct Client*)arg;
	struct Tcp_Msg temp;

	for (;;) {
		int r = recv(cli->tcp_connfd, (void*)&temp, sizeof(struct Tcp_Msg), 0);
		if (r == 0 || r == -1) {
			LOG("Client '%s' dissconneted\n", cli->usern);
			close(cli->tcp_connfd);
			atomic_store(&cli->state, CS_EMPTY);
			return NULL; 
		}

		switch (temp.m_type)
		{
			case NEW_CLIENT: {
				strncpy((char*)cli->usern, (char*)temp.data.id.username, USERNAME_MAX_LIMIT);
				atomic_store(&cli->state, CS_ACTIVE);
				break;
			}
		}
	}
}

void server_loop()
{
	socklen_t len = sizeof(struct sockaddr);

	for (;;) {
		assert(server.cli_amount != MAX_CLIENTS);
		struct sockaddr user;
		const int connfd = accept(server.sockfd, (struct sockaddr*)&user, &len);

		LOG("Accepted client '%s'\n", 
			inet_ntoa(((struct sockaddr_in*)&user)->sin_addr));

		if (connfd < 0) {
			ERROR("Failed call to accept\n");
			continue;
		}

		// if (client_count == MAX_CLIENTS) {
		// 	printf("MAX CLIENTS\n");
		// 	continue;

		unsigned int index = find_empty_client();
		server.client_list[index].tcp_connfd = connfd;
		pthread_create(&server.client_list[index].thread, 
					   NULL,
					   client_loop, 
					   &server.client_list[index]);
		server.cli_amount++;
	}
}

void server_cleanup()
{
	close(server.sockfd);
	free(server.client_list);
}

int main(int argc, const char** argv)
{
	if (!init_socket(SERVER_PORT, SERVER_IP))
		goto cleanup;

	if (!create_client_list())
		goto cleanup;

	LOG("Server Initialized %s:%d\n", SERVER_IP, SERVER_PORT);
	
	server_loop();

cleanup:
	server_cleanup();
	return 0;
}


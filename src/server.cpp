#include "network.hpp"
#include "config.hpp"

Server::Server(Network& net, const Client& cli) : net(net), cli(cli) {}

void Server::loop(Status& status)
{
	FD_ZERO(&master);
	FD_SET(net.conn.me(), &master);

	for (;;) {
		fd_set worker = master;

		timeval timeout = {1, 0};
		const i32 nready = select(FD_SETSIZE, &worker, NULL, NULL, &timeout);

		if (nready < 0) {
			P_ERROR("Error occured in select...\n");
		}
		else {
			if (status.get() == false)
				return;
		}

		for (i32 i = 0; i < nready; i++) 
		{
			if (FD_ISSET(net.conn.me(), &worker)) {
				accept_client();
			}			
			else {
				// for (const auto& c : db->client_list) {
				// 	if (c.state != Client::EMPTY && FD_ISSET(c.socket, &work_fds))
				// recv_msg(c.socket);
			}
		}		
	}
}

void Server::accept_client()
{
	struct sockaddr_in addr = {};
	socklen_t len = sizeof(addr);

	socket_t client_fd = accept(net.conn.me(), (sockaddr*)&addr, &len);

	if (db.full()) {
		LOGF("Client rejected '%s:%d'\n", inet_ntoa(addr.sin_addr), addr.sin_port);
		CLOSE_SOCKET(client_fd);
	}
	else {
		LOGF("Client Accepted '%s:%d'\n", inet_ntoa(addr.sin_addr), addr.sin_port);
		FD_SET(client_fd, &master);
		db.new_client(&addr, client_fd);
	}
}
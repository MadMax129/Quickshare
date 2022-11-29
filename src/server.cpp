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
			if (status.get() == false) {
				LOG("Exiting server loop...\n");
				return;
			}
		}

		for (i32 i = 0; i < nready; i++) {
			if (FD_ISSET(net.conn.me(), &worker)) 
				accept_client();		
			else
				read_msgs(worker);
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
		FD_SET(client_fd, &master);
		db.new_client(&addr, client_fd);
	}
}

void Server::read_msgs(fd_set& worker)
{
	auto can_recv = [&](Slot& slot) {
		if (slot.state != Slot::EMPTY && FD_ISSET(slot.sock, &worker))
			recv_msg(slot.sock);
	};

	db.iterate(can_recv);
}

void Server::recv_msg(const socket_t sock)
{
	Server_Msg msg;
	
	if (!net.conn.recv(sock, &msg)) {
		close_client(sock);
		return;
	}

	if (is_to_server(msg))
		analize_msg(sock, msg);
	else {
		// pass msg to client
		;
	}
}

bool Server::is_to_server(const Server_Msg& msg)
{
	(void)msg;
	return true;
}

void Server::analize_msg(const socket_t sock, Server_Msg& msg)
{
	switch (msg.type)
	{
		case Server_Msg::INIT_REQUEST:
			init_req(sock, msg);
			break;

		default: 
			break;
	}
}

void Server::init_req(const socket_t sock, Server_Msg& msg)
{
	const UserId id = db.complete_client(sock, msg.init_req.client_name);

	msg.type = Server_Msg::INIT_RESPONSE;
	msg.to = id;

	if (!net.conn.send(sock, &msg))
		close_client(sock);
}

void Server::close_client(socket_t sock)
{
	CLOSE_SOCKET(sock);
	FD_CLR(sock, &master);
	db.remove_client(sock);
}
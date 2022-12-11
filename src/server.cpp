#include "network.hpp"
#include "config.hpp"

Server::Server(Network& net, const Client* cli) : net(net), cli(cli), 
	db(net.get_db()), my_id(db.get_id()) {}

void Server::cleanup()
{
	LOG("Server cleanup...\n");
	auto close_all = [](const Slot& slot) {
		if (slot.state != Slot::EMPTY) {
			LOGF("Closing Client '%s'\n", 
				slot.state == Slot::COMPLETE ? slot.name : ""
			);
			CLOSE_SOCKET(slot.sock);
		}
	};
	db.iterate(close_all);
}

void Server::init()
{
	FD_ZERO(&master);
	FD_SET(net.conn.me(), &master);
}

void Server::loop(Status& status)
{
	for (;;) {
		worker = master;

		timeval timeout = {1, 0};
		const i32 nready = select(FD_SETSIZE, &worker, NULL, NULL, &timeout);

		if (nready < 0) {
			net.fail("Error occured in select...\n");
			return;
		}
		else if (status.get() == false) {
			LOG("Server loop close request... Exiting...\n");
			return;
		}

		for (i32 i = 0; i < nready; i++) {
			if (FD_ISSET(net.conn.me(), &worker)) 
				accept_client();		
			else
				read_msgs();
		}		
	}
}

void Server::accept_client()
{
	Sock_Info cli_info = net.conn.accept();

	if (!cli_info.has_value()) {
		net.fail("Accept failed\n");
		CLOSE_SOCKET(cli_info.value().first);
		return;
	}

	if (db.full()) {
		LOG("Client rejected, server full\n");
		CLOSE_SOCKET(cli_info.value().first);
	}
	else {
		FD_SET(cli_info.value().first, &master);
		db.new_client(
			&cli_info.value().second, 
			cli_info.value().first
		);
	}
}

void Server::read_msgs()
{
	// ? Optimize this evenutally
	auto can_recv = [&](const Slot& slot) {
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
	const UserId id = db.get_id();
	char temp_name[CLIENT_NAME_LEN];

	safe_strcpy(temp_name, msg.init_req.client_name, CLIENT_NAME_LEN);

	/* First confirm with client on ID */
	msg.type = Server_Msg::INIT_RESPONSE;
	msg.response.to = id;
	if (!net.conn.send(sock, &msg)) {
		close_client(sock);
		return;
	}

	/* Once confirmed create entry in DB and infrom others */
	db.complete_client(sock, temp_name, id);
	send_client_list(sock, msg);
	// ! ECHO BACK TO REST OF CLIENTS
}

void Server::send_client_list(const socket_t sock, Server_Msg& msg)
{
	msg.type = Server_Msg::NEW_CLIENT;

	/* Also manually send the server as a client */
	msg.cli_update.id = my_id;
	safe_strcpy(msg.cli_update.client_name, "Server", CLIENT_NAME_LEN);
	if (!net.conn.send(sock, &msg)) {
		close_client(sock);
		return;
	}

	auto send_new_clients = [&](const Slot& slot) {
		if (slot.state == Slot::COMPLETE && slot.sock != sock) {
			msg.cli_update.id = slot.id;
			safe_strcpy(
				msg.cli_update.client_name,
				slot.name,
				CLIENT_NAME_LEN
			);

			if (!net.conn.send(sock, &msg))
				close_client(sock);
		}
	};

	db.iterate(send_new_clients);
}

void Server::close_client(const socket_t sock)
{
	Slot client_slot;
	db.get_client(client_slot, sock);

	CLOSE_SOCKET(sock);
	FD_CLR(sock, &master);
	db.remove_client(sock);

	/* Conditions
		1. If client was not complete DO NOT send DELETE_CLIENT 
		2. If no more clients then also DO NOT send
	*/
	if (client_slot.state == Slot::COMPLETE && db.size() > 0)
		send_delete_client(client_slot);
}

void Server::send_delete_client(const Slot& client_slot)
{
	Server_Msg msg(Server_Msg::DELETE_CLIENT);
	safe_strcpy(msg.cli_update.client_name, client_slot.name, CLIENT_NAME_LEN);
	msg.cli_update.id = client_slot.id;
	
	auto update = [&](Slot& db_slot) {
		if (db_slot.state == Slot::COMPLETE) {
			if (!net.conn.send(db_slot.sock, &msg))
				close_client(db_slot.sock);
		}
	};

	db.iterate(update);
}
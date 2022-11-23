#include "network.hpp"
#include "config.hpp"

Server::Server(Network& net, const Client& cli) : net(net), cli(cli) {}

void Server::loop()
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
			// timeout
		}

		// pass worker as refenrece into server function and analize message

		// Read socket events
		for (i32 i = 0; i < nready; i++) 
		{
			if (FD_ISSET(net.conn.me(), &worker)) {
				LOG("HERE\n");
			}			
			else {
				// for (const auto& c : db->client_list) {
				// 	if (c.state != Client::EMPTY && FD_ISSET(c.socket, &work_fds))
				// recv_msg(c.socket);
			}
		}		
	}
}
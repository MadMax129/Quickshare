#include "network.hpp"
#include "config.hpp"
#include "mem_pool.hpp"

Network::Network()
{
	state.set(INACTIVE);

	server = reinterpret_cast<Server*>(mem_pool.alloc(sizeof(Server)));
	client = reinterpret_cast<Client*>(mem_pool.alloc(sizeof(Client)));

	new (server) Server(*this, client);
	new (client) Client(*this);
}

bool Network::get_ip(char ip_buffer[IP_ADDR_LEN])
{
#ifdef SYSTEM_WIN_64
	IP_ADAPTER_INFO FixedInfo;
	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);

	if (ERROR_SUCCESS != GetAdaptersInfo(&FixedInfo, &ulOutBufLen))
		return false;

	safe_strcpy(ip_buffer, FixedInfo.IpAddressList.IpAddress.String, IP_ADDR_LEN);

#elif 
#   error "Unimplemented 'get_ip' on this system client"
#endif
	return true;
}

void Network::fail(const char* str)
{
	P_ERROR(str);
	state.set(Network::FAIL_OCCURED);
}

void Network::init_network(bool is_server, const char ip[IP_ADDR_LEN])
{
	LOGF("Initializing Client/Server %s:%d\n", ip, STATIC_SERVER_PORT);
	safe_strcpy(this->ip, ip, IP_ADDR_LEN);

	thread_manager.new_thread(&Network::loop, this, is_server);
}

void Network::end()
{
	db.cleanup();
	conn.close();
}

void Network::loop(bool is_server, Status& status)
{
	if (!conn_setup(is_server)) {
		state.set(INIT_FAILED);
		return;
	}

	state.set(SUCCESS);

	if (is_server) {
		server->init();
		server->loop(status);
		server->cleanup();
	}
	else {
		client->loop(status);
	}

	/* Cleanup network once loop exists */
	end();
}

bool Network::conn_setup(bool is_server)
{
	if (conn.create_socket(ip, STATIC_SERVER_PORT)) {
		if (is_server && conn.bind_and_listen()) {
			return true;
		}
		else if (!is_server && conn.connect()) {
			conn.set_sock_timeout(conn.me(), 1);
			return true;
		}
	}
	else {
		return false;
	}

	P_ERROR("Failed connection setup\n");
	conn.close();

	return false;
}

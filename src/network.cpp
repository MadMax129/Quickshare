#include "network.hpp"
#include "config.hpp"

Thread_Manager thread_manager;

Network::Network(Locator& loc) : loc(loc), client(*this), server(*this, client) 
{
	state.set(INACTIVE);
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

void Network::init_network(bool is_server)
{
	LOGF("Initializing Client/Server %s:%d\n", loc.get_ip(), STATIC_SERVER_PORT);

	thread_manager.new_thread(&Network::loop, this, is_server);
}

void Network::loop(bool is_server, Status& status)
{
	if (!conn_setup(is_server)) {
		state.set(INIT_FAILED);
		return;
	}

	state.set(SUCCESS);

	if (is_server)
		server.loop(status);
	else
		client.loop(status);

	conn.close();
}

bool Network::conn_setup(bool is_server)
{
	if (conn.create_socket(loc.get_ip(), STATIC_SERVER_PORT)) {
		if (is_server && conn.bind_and_listen())
			return true;
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

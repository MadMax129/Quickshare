#include "network.hpp"
#include "config.hpp"
#include "mem_pool.hpp"

Network::Network()
{
	state.set(INACTIVE);

	client = reinterpret_cast<Client*>
		(mem_pool.alloc(sizeof(Client)));
	msg_buffer = reinterpret_cast<Server_Msg*>
		(mem_pool.alloc(sizeof(Server_Msg)));

	new (client) Client(*this, msg_buffer);
}

bool Network::get_ip(char ip_buffer[IP_ADDR_LEN])
{
#ifdef SYSTEM_WIN_64
	IP_ADAPTER_INFO FixedInfo;
	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);

	if (ERROR_SUCCESS != GetAdaptersInfo(&FixedInfo, &ulOutBufLen))
		return false;

	safe_strcpy(ip_buffer, FixedInfo.IpAddressList.IpAddress.String, IP_ADDR_LEN);
#elif defined(SYSTEM_UNX)
	(void)ip_buffer;
	return true;
#else
#   error "Unimplemented 'get_ip' on this system client"
#endif
	return true;
}

void Network::fail(const char* str)
{
	P_ERROR(str);
	state.set(Network::FAIL_OCCURED);
}

void Network::init_network(const char ip[IP_ADDR_LEN])
{
	LOGF("Initializing Client/Server %s:%d\n", ip, STATIC_SERVER_PORT);
	safe_strcpy(this->ip, ip, IP_ADDR_LEN);

	thread_manager.new_thread(&Network::loop, this);
}

void Network::end()
{
	db.cleanup();
	conn.close();
}

void Network::loop(Status& status)
{
	(void)status;
	// if (!conn_setup(is_server)) {
	// 	state.set(INIT_FAILED);
	// 	return;
	// }

	// state.set(SUCCESS);

	// if (is_server) {
	// 	server->init();
	// 	server->loop(status);
	// 	server->cleanup();
	// }
	// else {
	// 	client->loop(status);
	// }

	// /* Cleanup network once loop exists */
	// end();
}

bool Network::conn_setup()
{
	// if (conn.create_socket(ip, STATIC_SERVER_PORT)) {
	// 	if (is_server && conn.bind_and_listen()) {
	// 		return true;
	// 	}
	// 	else if (conn.connect()) {
	// 		conn.set_sock_timeout(conn.me(), 1);
	// 		return true;
	// 	}
	// }
	// else {
	// 	return false;
	// }

	// P_ERROR("Failed connection setup\n");
	// conn.close();

	return false;
}

#pragma once

#include "connection.hpp"
#include "../ip_server/include/ip_msg.hpp"
#include "state.hpp"
#include <mutex>

using Locator_Conn = Connection<Ip_Msg>;
using Key = char[SESSION_KEY_LEN];

struct Locator {
	enum State {
		/* Idle state  */
		INACTIVE,
		/* Doing work */
		WORKING,
		/* Failed to connect to QS Server */
		CONN_FAILED,
		/* Invalid session key */
		FAILED,
		/* Success completed request */
		SUCCESS
	};

	Locator();
	void locate(Key key);
	void create(Key key);

	State_Manager<State> state;

private:
	bool init_conn();
	void create_thread(Key key);

	Locator_Conn conn;
	char response[IP_ADDR_LEN];
};

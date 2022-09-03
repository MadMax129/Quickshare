#pragma once

#include "connection.hpp"
#include "../ip_server/ip_msg.hpp"
#include "state.hpp"
#include <mutex>

using Locator_Conn = Connection<Ip_Msg>;
using Key = char[NET_NAME_LEN];

struct Locator {
	enum State {
		/* Idle state  */
		INACTIVE,
		/* Doing work */
		WORKING,
		/* Failed to connect to QS Server */
		CONN_FAILED,
		/* Invalid session key */
		INVALID_KEY,
		/* Cannot create a session */
		KEY_FAILED,
		/* Success completed request */
		SUCCESS
	};

	void create(Key k);
	void locate(Key k);
	inline std::mutex& get_lock() { return lc; }
	inline State get_state() { return state; }

private:
	void c_loop(Key k);
	void l_loop(Key k);
	void end(State s);
	bool init();

	std::mutex lc;
	State state{INACTIVE};
	char response[MAX_IP_LEN];
	Locator_Conn conn;
};

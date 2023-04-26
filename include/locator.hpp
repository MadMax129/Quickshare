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

	enum Mode {
		/* Request to see if session id exists */
		LOCATE,
		/* Request to create new session id*/
		CREATE
	};

	Locator();

	void start(Mode mode, Key key);
	inline void reset() { state.set(INACTIVE); }

	State_Manager<State> state;

	inline const char* get_ip() const { return response; }

private:
	bool init_conn();
	bool contact(Ip_Msg* msg);
	void locate_thread(Key key);
	void create_thread(Key key);

	Locator_Conn conn;
	char response[IP_ADDR_LEN];
};

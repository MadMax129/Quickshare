#pragma once

#include "connection.hpp"
#include "../ip_server/ip_msg.hpp"
#include "state.hpp"

using Locator_Conn = Connection<Ip_Msg>;

struct Locator {
    enum State {
        INACTIVE,
    };

    bool init();
    bool locate();
    void cleanup();

private:
    State_Manager<State> state;
    Locator_Conn conn;
};

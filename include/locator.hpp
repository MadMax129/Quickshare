#pragma once

#include "connection.hpp"
#include "../ip_server/ip_msg.hpp"

using Locator_Conn = Connection<Ip_Msg>;

struct Locator {
    bool locate(Locator_Conn& lc);
    void cleanup();

private:
    Locator_Conn conn;
};

#pragma once

#include "quickshare.hpp"

struct Locator {
public:
    bool init_locator();

private:
    socket_t server_socket;
    struct sockaddr_in server_addr;
};

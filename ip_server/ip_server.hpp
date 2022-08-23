#pragma once

#ifndef __linux__
#   error Unsupported os
#endif

#include <cstdio>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sqlite3.h>
#include <cerrno>
#include <cstring>
#include <assert.h>
#include "ip_msg.hpp"
#include "config.hpp"

enum Host_State {
    // Default state, never should be set 
    EMPTY      = 0,

    // Server already present under host
    HAS_SERVER = 1,

    // Sender has become server
    YOU_SERVER = 2
};

struct Hosts {
    bool create_sql();
    void find_entry(const char* net_name, Ip_Msg* msg);
    void create_entry(const char* net_name, const char* ip, Ip_Msg* msg);
    void end();

private:
    sqlite3* db;
};

struct Server {
    Server(Hosts& hosts) : hosts(hosts) {}
    bool init_server();
    void loop();
    void end();
private:
    void request();
    void handle_msg(Ip_Msg& msg);
    void send_msg(Ip_Msg* msg);

    Hosts& hosts;
    int sock, new_client;
    sockaddr_in addr, new_addr;
};

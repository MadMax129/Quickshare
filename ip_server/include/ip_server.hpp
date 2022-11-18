#pragma once

#ifndef __linux__
#   error Unsupported os
#endif

#include <cstdio>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sqlite3.h>
#include <cerrno>
#include <cstring>
#include <assert.h>
#include <thread>
#include <condition_variable>
#include <atomic>
#include "ip_msg.hpp"

#define DATABASE_PATH "./database/ip_list_db"

enum Host_State {
    // Default state, never should be set 
    EMPTY      = 0,

    // Server already present under host
    HAS_SERVER = 1,

    // Sender has become server
    YOU_SERVER = 2
};

enum Server_State {
    ONLINE,
    SHUTDOWN
};

struct Hosts {
    bool create_sql();
    void find_entry(const char* net_name, Ip_Msg* msg);
    void create_entry(const char* net_name, const char* ip, Ip_Msg* msg);
    void end();
    std::condition_variable cond;

private:
    void cleaner();
    std::thread c_th;
    std::mutex c_mtx;
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

extern std::atomic<Server_State> global_state;

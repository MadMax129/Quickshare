#include "ip_server.hpp"
#include <signal.h>
#include <cstdlib>

std::atomic<Server_State> global_state{ONLINE};
static Hosts hosts;
static Server server(hosts);

void signal_callback_handler(int signum) 
{
    (void)signum;
    std::printf("Forced shutdown\n");
    global_state.store(SHUTDOWN, std::memory_order_relaxed);
    hosts.cond.notify_one();
    server.end();
}

int main() 
{
    signal(SIGINT, signal_callback_handler);

    if (!hosts.create_sql()) {
        std::printf("Error to init sql...\n");
        return 1;
    }

    if (!server.init_server()) {
        std::printf("Init server failed\n");
        return 1;
    }

    server.loop();

    server.end();
    hosts.end();

    return 0;
}
#include "ip_server.hpp"
#include <signal.h>
#include <cstdlib>

static Hosts hosts;
static Server server(hosts);

void signal_callback_handler(int signum) 
{
    (void)signum;
    std::printf("Forced shutdown\n");
    server.end();
    hosts.end();
    std::exit(1);
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
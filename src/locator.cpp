#include "locator.hpp"
#include "config.hpp"
#include <cstring>

bool Locator::init()
{
    if (!conn.create_socket(STATIC_QS_SERVER_IP, STATIC_QS_SERVER_PORT))
        return false;

    if (!conn.connect())
        return false;

    LOG("Connected to Qs Server...\n");

    state.set(INACTIVE);

    return true;
}

bool Locator::locate()
{
    Ip_Msg msg;
    msg.type = Ip_Msg::ADD;
    std::strcpy(msg.request.my_ip, "192.168.1.2");
    std::strcpy(msg.request.net_name, "my_net_123");

    assert(conn.send(conn.me(), &msg));
    assert(conn.recv(conn.me(), &msg));
    assert(msg.type != Ip_Msg::INVALID);

    return true;
}

void Locator::cleanup()
{
    conn.close();
}
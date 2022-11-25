#include "locator.hpp"
#include "config.hpp"
#include "network.hpp"
#include <cstring>
#include <thread>

Locator::Locator()
{
    LOG("Locator Initialized...\n");
    state.set(INACTIVE);
}

void Locator::start(Mode mode, Key key)
{
    LOG("Locator Started!\n");
    state.set(WORKING);

    auto func = (mode == LOCATE) ? 
        &Locator::locate_thread : 
        &Locator::create_thread;
    std::thread th(func, this, key);
    th.detach();
}

void Locator::locate_thread(Key key)
{
    Ip_Msg msg(Ip_Msg::REQUEST, key, NULL);

    if (!contact(&msg))
        return;

    switch (msg.type)
    {
        case Ip_Msg::Type::RESPONSE:
            safe_strcpy(response, msg.response.ip, IP_ADDR_LEN);
            state.set(SUCCESS, std::memory_order_release);
            LOGF("Response: VALID\n\t%s\n", response);
            break;

        default:
            P_ERRORF("Invalid key '%s'\n", key);
            state.set(FAILED);
            break;
    }

    conn.close();
}

void Locator::create_thread(Key key)
{
    char ip[16];
    
    if (!Network::get_ip(ip)) {
        state.set(CONN_FAILED);
        return;
    }
    
    Ip_Msg msg(Ip_Msg::CREATE, key, ip);
    
    if (!contact(&msg))
        return;

    switch (msg.type)
    {
        case Ip_Msg::Type::RESPONSE:
            LOGF("Created key '%s'\n", key);
            safe_strcpy(response, ip, IP_ADDR_LEN);
            state.set(SUCCESS);
            break;

        default:
            P_ERRORF("Cannot create key '%s'\n", key);
            state.set(FAILED);
            break;
    }

    conn.close();
}

bool Locator::init_conn()
{
    if (!conn.create_socket(STATIC_QS_SERVER_IP, STATIC_QS_SERVER_PORT))
        return false;

    if (!conn.connect())
        return false;

    LOG("Connected to Qs Server...\n");

    return true;
}

bool Locator::contact(Ip_Msg* msg)
{
    if (!init_conn() || !conn.send_and_recv(conn.me(), msg)) {
        conn.close();
        state.set(CONN_FAILED);
        return false;
    }

    return true;
}
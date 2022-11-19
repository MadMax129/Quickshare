#include "locator.hpp"
#include "config.hpp"
#include <cstring>
#include <thread>

Locator::Locator()
{
    state.set(INACTIVE);
}

void Locator::locate(Key key)
{
    state.set(WORKING);

    if (!init_conn()) {
        conn.close();
        state.set(CONN_FAILED);
        return;
    }

    std::thread loc(&Locator::create_thread, this, key);

    loc.detach();
}

void Locator::create_thread(Key key)
{
    Ip_Msg msg;
    msg.type = Ip_Msg::REQUEST;
    std::strncpy(msg.request.net_name, key, SESSION_KEY_LEN);
    msg.request.net_name[SESSION_KEY_LEN - 1] = '\0';

    // Send request
    if (!conn.send(conn.me(), &msg)) {
        conn.close();
        state.set(CONN_FAILED);
        return;
    }

    // Wait for reply
    if (!conn.recv(conn.me(), &msg)) {
        conn.close();
        state.set(CONN_FAILED);
        return;
    }

    switch (msg.type)
    {
        case Ip_Msg::Type::RESPONSE:
            std::strncpy(response, msg.response.ip, IP_ADDR_LEN);
            response[IP_ADDR_LEN - 1] = '\0';
            state.set(SUCCESS, std::memory_order_release);
            LOGF("Response: VALID\n\t%s\n", response);
            break;

        default:
            LOG("Response: INVALID\n");
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
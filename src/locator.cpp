#include "locator.hpp"
#include "config.hpp"
#include <cstring>
#include <thread>

bool Locator::init()
{
    if (!conn.create_socket(STATIC_QS_SERVER_IP, STATIC_QS_SERVER_PORT))
        return false;

    if (!conn.connect())
        return false;

    LOG("Connected to Qs Server...\n");

    return true;
}

void Locator::c_loop(Key k)
{
    (void)k;
}

void Locator::end(State s)
{
    state = s;
    conn.close();
    lc.unlock();
}

void Locator::l_loop(Key k)
{
    Ip_Msg msg;
    if (!init()) {
        end(CONN_FAILED);
        return;
    }
    
    msg.type = Ip_Msg::REQUEST;
    std::snprintf(msg.request.net_name, NET_NAME_LEN, "%s", k);

    if (!conn.send(conn.me(), &msg)) {
        end(CONN_FAILED);
        return;
    }

    if (!conn.recv(conn.me(), &msg)) {
        end(CONN_FAILED);
        return;
    }

    if (msg.type != Ip_Msg::RESPONSE) {
        end(INVALID_KEY);
        return;
    }
    else {
        snprintf(response, sizeof(response), "%s", msg.response.ip);
    }
    
    end(SUCCESS);
}

void Locator::locate(Key k)
{
    if (lc.try_lock()) {
        state = WORKING;
        std::thread l_th(&Locator::l_loop, this, k);
        l_th.detach();
    }
}

void Locator::create(Key k)
{
    if (lc.try_lock()) {
        state = WORKING;
        std::thread c_th(&Locator::c_loop, this, k);
        c_th.detach();
    }
}
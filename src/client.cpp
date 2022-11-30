#include "network.hpp"
#include "config.hpp"

Client::Client(Network& net) : net(net) {}


void Client::loop(Status& status)
{
    Server_Msg msg;

    if (!init(msg)) {
        P_ERROR("Client init msg failed to sent...\n");
        net.state.set(Network::FAIL_OCCURED);
        return;
    }

    while (status.get()) {
        bool good = net.conn.recv(net.conn.me(), &msg);
        
        // Timeout
        // TODO ADD LINUX SUPPORT in connection
        if (!good && WSAGetLastError() == WSAETIMEDOUT) 
            continue;

        if (!good) {
            P_ERROR("Recv failed...\n");
            net.state.set(Network::FAIL_OCCURED);
            return;
        }

        analize_msg(msg);
    }
}

bool Client::init(Server_Msg& msg)
{
    msg.type = Server_Msg::INIT_REQUEST;
    get_computer_name(msg.init_req.client_name);

    return net.conn.send(net.conn.me(), &msg);
}

void Client::analize_msg(Server_Msg& msg)
{
    switch (msg.type)
    {
        case Server_Msg::INIT_RESPONSE:
            init_res(msg);
            break;

        case Server_Msg::DELETE_CLIENT:
            printf("Got delete client\n");
            break;

        default:
            break;
    }
}

void Client::init_res(const Server_Msg& msg)
{
    my_id = msg.to;
}
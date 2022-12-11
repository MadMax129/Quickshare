#include "network.hpp"
#include "config.hpp"

Client::Client(Network& net) : net(net) {}

void Client::loop(Status& status)
{
    Server_Msg msg;

    if (!init(msg)) {
        net.fail("Client failed to send init msg...\n");
        return;
    }

    while (status.get()) {
        bool good = net.conn.recv(net.conn.me(), &msg);
        
        // Timeout
        // TODO ADD LINUX SUPPORT in connection
        if (!good && WSAGetLastError() == WSAETIMEDOUT) 
            continue;

        if (!good) {
            net.fail("Recv failed...\n");
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

        case Server_Msg::NEW_CLIENT:
            printf("Got new client '%s'\n", msg.cli_update.client_name);
            break;

        default:
            break;
    }
}

void Client::init_res(const Server_Msg& msg)
{
    my_id = msg.response.to;
}
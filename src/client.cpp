#include "network.hpp"
#include "config.hpp"

Client::Client(Network& net, Server_Msg* msg_buf) 
    : net(net), msg_buf(msg_buf) {}

void Client::loop(Status& status)
{
    if (!init()) {
        net.fail("Client failed to send init msg...\n");
        return;
    }

    while (status.get()) {
        bool good = net.conn.recv(net.conn.me(), msg_buf);
        
        // Timeout
        // TODO ADD LINUX SUPPORT in connection
        if (!good && WSAGetLastError() == WSAETIMEDOUT) 
            continue;

        if (!good) {
            net.fail("Recv failed...\n");
            return;
        }

        analize_msg();
    }
}

bool Client::init()
{
    new (msg_buf) Server_Msg(Server_Msg::INIT_REQUEST);
    get_computer_name(msg_buf->d.init_req.client_name);

    return net.conn.send(net.conn.me(), msg_buf);
}

void Client::analize_msg()
{
    switch (msg_buf->type)
    {
        case Server_Msg::INIT_RESPONSE:
            init_res();
            break;

        case Server_Msg::DELETE_CLIENT:
            printf("Got delete client\n");
            break;

        case Server_Msg::NEW_CLIENT:
            printf("Got new client '%s'\n", msg_buf->d.cli_update.client_name);
            break;

        default:
            break;
    }
}

void Client::init_res()
{
    my_id = msg_buf->d.response.to;
}
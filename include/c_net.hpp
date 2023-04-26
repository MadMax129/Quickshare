#pragma once

/* A client may be the server (if created session)
    This means that server is either used or not
*/

class Network;

class Client {
public:
    Client(Network& net, Server_Msg* msg);

    void loop(Status& status);

private:
    bool init();
    void analize_msg();
    
    /* Server Message Analysis */
    void init_res();

    UserId my_id;
    Network& net;
    Server_Msg* const msg_buf;
};
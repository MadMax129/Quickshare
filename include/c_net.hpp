#pragma once

/* A client may be the server (if created session)
    This means that server is either used or not
*/

class Network;

class Client {
public:
    Client(Network& net);

    void loop(Status& status);

private:
    bool init();
    void analize_msg(Server_Msg& msg);
    
    /* Server Message Analysis */
    void init_res(const Server_Msg& msg);

    UserId my_id;
    Network& net;
};
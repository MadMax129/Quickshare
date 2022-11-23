#pragma once

/* A client may be the server (if created session)
    This means that server is either used or not
*/

class Network;

class Client {
public:
    Client(Network& net);

    void loop();

private:
    Network& net;
};
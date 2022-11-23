#pragma once

class Network;
class Client;

#include "connection.hpp"

class Server {
public:
    Server(Network &net, const Client& cli);

    void loop();

private:
    void accept_client();

    fd_set master; // TODO Consider alloctating these later on
    Network &net;
    const Client& cli;
};
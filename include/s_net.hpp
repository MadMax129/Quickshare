#pragma once

class Network;
class Client;

#include "connection.hpp"
#include "thread_manager.hpp"
#include "database.hpp"

class Server {
public:
    Server(Network &net, const Client& cli);

    void loop(Status& status);

private:
    void accept_client();

    Database db;
    fd_set master; // TODO Consider alloctating these later on
    Network &net;
    const Client& cli;
};
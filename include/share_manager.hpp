#pragma once

#include "network.hpp"
#include "connection.hpp"
#include "data_packet.hpp"
#include "file_manager.hpp"
#include <atomic>

class Share_Manager {
public:
    Share_Manager(Network& net) : net(net) {}

private:
    struct Session {
        Connection<Data_Packet> conn;
        File_Manager file;
        std::atomic<u64> progress;
    };

    Session sender;
    Session reciever;
    Network& net;
};
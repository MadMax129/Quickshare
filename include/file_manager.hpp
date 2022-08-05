#pragma once

#include "network.hpp"
#include "quickshare.hpp"
#include <mutex>
#include <cstdio>
#include <cmath>
#include <list>
#include <utility>
#include <condition_variable>
#include "../lib/SPSCQueue.h"

#define MAX_RECV_QUEUE_SIZE 32

using Users_List = std::list<std::pair<UserId, Msg::Msg_Type>>;

struct File_Sharing {
    File_Sharing();
    ~File_Sharing();

    enum Transfer_State {
        INACTIVE,
        REQUESTED,
        ACTIVE,
        FINISHED,
        FAILED
    };

    struct Transfer_Data {
        std::atomic<Transfer_State> state;
        std::atomic<u32> progress;
        Request hdr;
        std::FILE* file;
        Users_List users;
        std::thread thread;
        std::mutex lock;
        std::condition_variable cond;
    };

    void cleanup();
    inline void add_network(Network* net) { network = net; }
    bool create_send(const char* fname, Users_List users);
    bool create_recv(const Request *r_hdr, UserId from);
    void fail_recv();
    void accept_request();
    void got_response(const Msg* msg);
    void push_msg(const Msg* msg);

    // Make private add api to control
    Transfer_Data s_data{};
    Transfer_Data r_data{};

private:
    Msg* temp_msg;
    // Queue r_msg_queue;
    rigtorp::SPSCQueue<Msg> r_msg_queue;
    void send_loop();
    void send_packets();
    void recv_loop();

    Network* network;
};
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
#include "allocation.hpp"

#define MAX_RECV_QUEUE_SIZE 64

using Users_List = std::list<std::pair<UserId, Msg::Msg_Type>>;

struct File_Sharing {
    File_Sharing();
    ~File_Sharing();

    enum Transfer_State {
        /* Default state, not doing anything */
        INACTIVE,

        /* Awaiting more info
            send_loop()
                - Waiting for responses from clients

            cli_loop()
                - Waiting for accept from gui
        */
        REQUESTED,

        /* Actively sending or receving */
        ACTIVE,

        /* Successfully finished send or recv */
        FINISHED,

        /* Failed to send or recv file */
        FAILED,

        /* Program requestes to kill the thread */
        KILL,

        /* Close current transfer session but not thread */
        CLOSE
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
        u8* buffer;
    };

    void cleanup();
    inline void add_network(Network* net) { network = net; }
    bool create_send(const char* fname, Users_List users);
    bool create_recv(const Request *r_hdr, UserId from);

    inline void set_send(Transfer_State state)
    {
        s_data.state.store(state, std::memory_order_relaxed);
    }

    inline void set_recv(Transfer_State state)
    {
        r_data.state.store(state, std::memory_order_relaxed);
    }
    
    void accept_request();
    void got_response(const Msg* msg);
    void push_msg(const Msg* msg);

    // Make private add api to control
    Transfer_Data s_data{};
    Transfer_Data r_data{};

private:
    Msg* temp_msg;
    rigtorp::SPSCQueue<Msg> r_msg_queue;

    void send_loop();
    void send_requests();
    bool got_all_responses();

    void send_packets();
    void recv_loop();
    u64 packets_in_file(u64 file_size);

    Network* network;
};
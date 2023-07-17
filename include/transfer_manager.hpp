#pragma once

#include <atomic>
#include <mutex>
#include <vector>
#include <filesystem>
#include <string>

#include "msg.h"
#include "network.hpp"
#include "connection.hpp"
#include "file_manager.hpp"
#include "LockFreeQueueCpp11.h"

struct Active_Transfer {
    enum Type {
        /* Transfer created by client */
        TRANSFER_HOST,
        /* Transfer recv from other */
        TRANSFER_RECV
    };

    enum State {
        EMPTY,
        PENDING,
    };

    Active_Transfer() :
        state(EMPTY),
        hdr({}),
        local_id(0)
    {}

    Type type;
    std::atomic<State> state;
    Transfer_Hdr hdr;
    Transfer_ID local_id;
    std::filesystem::path file;
};

struct Transfer_Info {
    enum State {
        CANCELLED,
        COMPLETE
    };

    State state;
    std::filesystem::path file;
};

struct Transfer_Cmd {
    enum Type {
        REQUEST,
        RESPONSE,
        CANCEL
    };

    Type type;
    Transfer_ID t_id;
    
    union {
        bool reply;
        Transfer_Request req;
    } cmd;
};

using Transfer_Vec = std::vector<Transfer_Info>;

class Transfer_Manager {
public:
    static Transfer_Manager& get_instance() {
        static Transfer_Manager instance;
        return instance;
    }

    void process_cmds();
    bool server_request(const Transfer_Request* request);
    // void server_accept();

    inline bool client_cmd(const Transfer_Cmd& t_cmd)
    {
        return cmd_queue.push(t_cmd);
    }

    // void create_recv_request(const Transfer_Request* request);

    // void request_response(const Transfer_Request* response, const bool valid);
    // void got_request_reply(const Transfer_Request* transfer, const bool reply);
    // void send_reply(const Transfer_ID t_id, const bool reply);

    // bool write_packet(Packet* packet);

    // inline void copy(Transfer_Vec& t_vec)
    // {
    //     if (dirty.load(std::memory_order_acquire)) {
    //         if (t_lock.try_lock()) {
    //             t_vec = transfers;
    //             dirty.store(false, std::memory_order_release);
    //             t_lock.unlock();
    //         }
    //     }
    // }

private:
    Transfer_Manager();
    Transfer_Manager(const Transfer_Manager&) = delete;
    Transfer_Manager& operator=(const Transfer_Manager&) = delete;

    Active_Transfer* get_transfer(const Active_Transfer::Type type);
    // inline void push_transfer(Transfer& transfer)
    // {
    //     std::lock_guard<std::mutex> lock(t_lock);
    //     transfers.push_back(transfer);
    //     dirty.store(true, std::memory_order_release);
    // }

    // void send_req(Transfer& transfer, Packet* packet);
    // void send_reply(Transfer& transfer, Packet* packet);

    std::mutex backlog_mtx;
    Transfer_Vec backlog;
    std::atomic<bool> dirty;
    Transfer_ID client_transfer_ids;
    LockFreeQueueCpp11<Transfer_Cmd> cmd_queue;
    std::atomic<u32> t_count;
    std::array<Active_Transfer, SIM_TRANSFERS_MAX> transfers;
};
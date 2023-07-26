#pragma once

#include <atomic>
#include <mutex>
#include <vector>
#include <filesystem>
#include <string>

#include "msg.h"
#include "util.h"
#include "network.hpp"
#include "connection.hpp"
#include "file_manager.hpp"
#include "LockFreeQueueCpp11.h"

struct Active_Transfer {
    enum State {
        EMPTY,
        CANCEL,
        SEND_REQ,
        GET_RESPONSE,
        ACCEPT,
        DENY,
        PENDING,
        ACTIVE
    };

    Active_Transfer() :
        state(EMPTY),
        hdr({}),
        accept_list{},
        local_id(0)
    {}

    std::atomic<State> state;
    Transfer_Hdr hdr;
    State accept_list[TRANSFER_CLIENTS_MAX];
    Transfer_ID local_id;
    std::filesystem::path file;
};

struct Transfer_Info {
    enum State {
        CANCELLED,
        DENIED,
        REJECTED,
        COMPLETE
    };

    State state;
    std::filesystem::path file;
};

struct Transfer_Cmd {
    enum Type {
        REQUEST,
        REPLY,
        CANCEL
    };

    Transfer_Cmd() = default;
    Transfer_Cmd(
        const char* filepath, 
        const Client_ID to[TRANSFER_CLIENTS_MAX]
    ) {
        type = REQUEST;
        safe_strcpy(
            this->d.req.filepath,
            filepath,
            FILE_NAME_LEN * 2
        );
        (void)std::memcpy(
            d.req.to,
            to,
            sizeof(d.req.to)
        );
    }
    Transfer_Cmd(
        const Transfer_ID t_id,
        const bool reply
    ) {
        type = REPLY;
        this->t_id = t_id;
        this->d.reply = reply;
    }

    Type type;
    Transfer_ID t_id;
    union {
        bool reply;
        struct {
            char filepath[FILE_NAME_LEN * 2];
            Client_ID to[TRANSFER_CLIENTS_MAX];
        } req;
    } d;
};

using Transfer_Vec = std::vector<Transfer_Info>;

class Transfer_Manager {
public:
    static Transfer_Manager& get_instance() {
        static Transfer_Manager instance;
        return instance;
    }

    /* Create a RECV Request (S) */
    bool create_recv_request(const Transfer_Request* request);
    void recv_cancel();

    /* Host request confimration (S) */
    void host_request_valid(const Transfer_Request* req, const bool reply);
    void host_request_reply(
        const Transfer_ID t_id, 
        const Client_ID c_id, 
        const bool reply
    );
    void host_cancel();

    using Transfer_Array = std::array<Active_Transfer, SIM_TRANSFERS_MAX>;

    inline const Transfer_Array& get_host_transfers() const
    {
        return host_transfers;
    }

    inline const Transfer_Array& get_recv_transfers() const 
    {
        return recv_transfers;
    }

    inline bool send_cmd(Transfer_Cmd& cmd) 
    {
        return cmd_queue.push(cmd);
    }

    bool do_work(Packet* packet);

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

    void process_cmds();
    void cmd_host_request(
        const char* path, 
        const Client_ID c_ids[TRANSFER_CLIENTS_MAX]
    );
    void cmd_recv_request_reply(
        const Transfer_ID t_id, 
        const bool reply
    );
    void set_request(
        Active_Transfer& transfer,
        const Client_ID c_id,
        const bool reply
    );
    Active_Transfer* get_transfer(Transfer_Array& t_array);
    void host_cleanup(Active_Transfer& transfer);
    void recv_cleanup();

    void send_request(Active_Transfer& transfer, Packet* packet);
    void send_recv_request_reply(Active_Transfer& transfer, Packet* packet);

    bool host_transfer_work(Packet* packet);
    bool recv_transfer_work(Packet* packet);

    using Cmd_Queue = LockFreeQueueCpp11<Transfer_Cmd>;

    std::mutex backlog_mtx;
    Transfer_Vec backlog;
    std::atomic<bool> dirty;
    Transfer_ID client_transfer_ids;
    Cmd_Queue cmd_queue;
    Transfer_Array recv_transfers;
    Transfer_Array host_transfers;
};
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

    inline bool can_cancel() const {
        const State state = this->state.load(
            std::memory_order_acquire
        );
        return state == ACTIVE  || 
               state == PENDING ||
               state == GET_RESPONSE; 
    }

    std::atomic<State> state;
    Transfer_Hdr hdr;
    State accept_list[TRANSFER_CLIENTS_MAX];
    Transfer_ID local_id;
    std::chrono::high_resolution_clock::time_point past_send;
    std::filesystem::path file;
    i64 file_size;
    File_Manager f_manager;
};

struct Transfer_Info {
    enum State {
        CANCELLED,
        DENIED,
        REJECTED,
        COMPLETE
    };

    Transfer_Info(
        const State state,
        const std::filesystem::path file
    ) : 
    state(state),
    file(file)
    {}

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
        (void)std::memcpy(
            this->d.req.filepath,
            filepath,
            std::min(
                strlen(filepath) + 1,
                sizeof(this->d.req.filepath)
            )
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

    Transfer_Cmd(
        const Transfer_ID t_id
    ) {
        type = CANCEL;
        this->t_id = t_id;
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
    enum Work_State {
        HAS_WORK,
        NO_WORK,
        WAIT_WORK
    };

    static Transfer_Manager& get_instance() {
        static Transfer_Manager instance;
        return instance;
    }

    /* Create a RECV Request (S) */
    bool create_recv_request(const Transfer_Request* request);
    void recv_cancel(const Transfer_Hdr* hdr);
    void recv_data(const Transfer_Data* t_data);
    void recv_complete(const Transfer_ID t_id);

    /* Host request confimration (S) */
    void host_request_valid(const Transfer_Request* req, const bool reply);
    void host_request_reply(
        const Transfer_ID t_id, 
        const Client_ID c_id, 
        const bool reply
    );

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

    Work_State do_work(Packet* packet);

    inline void copy(Transfer_Vec& t_vec)
    {
        if (dirty.load(std::memory_order_acquire)) {
            if (backlog_mtx.try_lock()) {
                t_vec = backlog;
                dirty.store(false, std::memory_order_release);
                backlog_mtx.unlock();
            }
        }
    }

private:
    Transfer_Manager();
    Transfer_Manager(const Transfer_Manager&) = delete;
    Transfer_Manager& operator=(const Transfer_Manager&) = delete;

    inline void push_backlog(
        const Transfer_Info::State state,
        const std::filesystem::path file 
    ) 
    {
        backlog.emplace_back(state, file);
        dirty.store(true, std::memory_order_release);
    }

    void zero_transfer(Active_Transfer& transfer);
    void process_cmds();
    void cmd_host_request(
        const char* path, 
        const Client_ID c_ids[TRANSFER_CLIENTS_MAX]
    );
    void cmd_recv_request_reply(
        const Transfer_ID t_id, 
        const bool reply
    );
    void cmd_cancel(const Transfer_ID t_id);
    void set_request(
        Active_Transfer& transfer,
        const Client_ID c_id,
        const bool reply
    );
    Active_Transfer* get_transfer(Transfer_Array& t_array);
    void handle_transfer_data(
        Active_Transfer& transfer,
        const Transfer_Data* t_data
    );

    void send_cancel(Active_Transfer& transfer, Packet* packet);
    void send_request(Active_Transfer& transfer, Packet* packet);
    void send_recv_request_reply(Active_Transfer& transfer, Packet* packet);
    Work_State send_data(Active_Transfer& transfer, Packet* packet);

    Work_State host_transfer_work(Packet* packet);
    Work_State recv_transfer_work(Packet* packet);

    using Cmd_Queue = LockFreeQueueCpp11<Transfer_Cmd>;

    std::mutex backlog_mtx;
    Transfer_Vec backlog;
    std::atomic<bool> dirty;
    Transfer_ID client_transfer_ids;
    Cmd_Queue cmd_queue;
    Transfer_Array recv_transfers;
    Transfer_Array host_transfers;
};
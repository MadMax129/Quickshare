#pragma once

#include <atomic>
#include <mutex>
#include <vector>

#include "msg.h"
#include "network.hpp"
#include "connection.hpp"
#include "file_manager.hpp"
#include <filesystem>
#include <string>

enum Transfer_Type {
    /* Transfer created by client */
    TRANSFER_HOST,
    /* Transfer recv from other */
    TRANSFER_RECV
};

enum Transfer_State {
    SEND_REQ,
    PENDING,
    CANCELLED,
    ACTIVE,
    ERROR,
    COMPLETE
};

struct Transfer {
    Transfer(Transfer_Type type) : 
        type(type),
        t_hdr({}),
        client_t_id(0) 
    {}

    Transfer_Type type;
    Transfer_State state;
    Transfer_Hdr t_hdr;
    Transfer_ID client_t_id;
    File_Manager::Session* session;
    std::filesystem::path file_path;
};

using Transfer_Vec = std::vector<Transfer>;

class Transfer_Manager {
public:

    static Transfer_Manager& get_instance() {
        static Transfer_Manager instance;
        return instance;
    }

    bool create_host_request(
        const char* path,
        const Transfer_ID t_id[TRANSFER_CLIENTS_MAX]
    );

    void create_request(const Transfer_Request* request);
    bool write_packet(Packet* packet);

    inline void copy(Transfer_Vec& t_vec)
    {
        if (dirty.load(std::memory_order_acquire)) {
            if (t_lock.try_lock()) {
                t_vec = transfers;
                dirty.store(false, std::memory_order_release);
                t_lock.unlock();
            }
        }
    }

    /* API
    
    create transfer request()
        * Either await server reponse 
            or client response

        * return server transfer_id
            or client side transfer_id

    accept transfer()
        * Called by server message reply
            or by client GUI

        * IF server message then look for 
            last client transfer made

        * IF gui response look for transfer_id

    draw()
        * GUI draw all active transfer and state in 
            sessions

    cancel transfer()
        call at any point to cancel 
        * Either server or client  
    
    */


private:
    Transfer_Manager();
    Transfer_Manager(const Transfer_Manager&) = delete;
    Transfer_Manager& operator=(const Transfer_Manager&) = delete;

    inline void push_transfer(Transfer& transfer)
    {
        std::lock_guard<std::mutex> lock(t_lock);
        transfers.push_back(transfer);
        dirty.store(true, std::memory_order_release);
    }

    File_Manager f_manager;
    std::mutex t_lock;
    Transfer_Vec transfers;
    Transfer_ID client_transfer_ids;
    std::atomic<bool> dirty;
};
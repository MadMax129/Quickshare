#pragma once

#include <atomic>
#include <vector>

#include "msg.h"
#include "network.hpp"
#include "connection.hpp"
#include "file_manager.hpp"

class Transfer_Manager {
public:
    enum Transfer_Type {
        /* Transfer created by client */
        TRANSFER_HOST,
        /* Transfer recv from other */
        TRANSFER_RECV
    };

    enum Transfer_State {
        PENDING,
        CANCELLED,
        ACTIVE,
        ERROR,
        COMPLETE
    };

    struct Transfer {
        Transfer(Transfer_Type type) : 
            type(type), 
            state(PENDING),
            t_hdr({}),
            client_t_id(0) {}
    
        const Transfer_Type  type;
        Transfer_State state;
        Transfer_Hdr   t_hdr;
        Transfer_ID    client_t_id;
        File_Manager   file_session;
    };

    static Transfer_Manager& get_instance() {
        static Transfer_Manager instance;
        return instance;
    }

    void create_request();
    void create_request(const Transfer_ID t_id);

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

    std::vector<Transfer> transfers;
    Transfer_ID client_transfer_ids;
};
#pragma once

#include <atomic>

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
        Transfer_Type type;
        Transfer_State state;
        Transfer_Request info;
        uint64_t progress;
        // file manager thing
    };

    Transfer_Manager() {}

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
    /* File_Manager */




};
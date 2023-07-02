#pragma once

#include "network.hpp"
#include "connection.hpp"
#include "file_manager.hpp"
#include <atomic>

class Transfer_Manager {
public:
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
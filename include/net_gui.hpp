#pragma once

#include "msg.hpp"
#include <cstring>

/* Network-gui queue size */
#define NETWORK_GUI_QUEUE_SIZE 32

struct Net_Gui_Msg {
    Net_Gui_Msg(Msg::Msg_Type type) 
    {
        this->type = type;
    }

    void copy_list(Msg::Client_List* list)
    {
        std::memcpy(
            &this->list, 
            list, 
            sizeof(Msg::Client_List)
        );
    }
    
    Msg::Msg_Type type;
    union {
        Msg::Client_List list;
    };
};
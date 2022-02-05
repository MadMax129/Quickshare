#pragma once

#include "networking.h"
#include <vector>

struct Context;

struct Chat_Menu {
    Chat_Menu(Context* context);
    ~Chat_Menu();
    
    void test();
    void draw();

private:
    void update_msgs();

    Context* ctx;
    std::vector<Tcp_Msg::Chat_Msg> msgs;
    std::unique_ptr<Tcp_Msg> msg_buf;
    static const unsigned int MAX_MSG_AMT = 60;
};
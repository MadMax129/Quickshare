#pragma once

class Context;

#include "imgui.h"

using Key = char[16];

class Login_Menu {
public: 
    Login_Menu(Context& context);
    
    void draw();
    void clean();

private:
    void draw_inner();
    void draw_key();
    void draw_enter();
    void draw_text();
    void button();
    void check_state();
    void loc_check();
    void net_check();

    Context& ctx;
    enum {
        /* No ongoing tasks */
        IDLE,
        CONNECTING,
        NETWORK_ERROR
    } state;
    Key key;
    ImVec2 inner_size;
    bool login_state;
    const char* error;
};
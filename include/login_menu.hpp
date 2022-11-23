#pragma once

struct Context;

#include "imgui.h"
#include "locator.hpp"

struct Login_Menu {
public: 
    Login_Menu(Context& context);
    void draw();

private:
    void draw_inner();
    void draw_key();
    void draw_enter();
    void draw_text();
    void check_state();
    void loc_check();
    void net_check();

    Context& ctx;
    enum {
        /* No ongoing tasks */
        IDLE,
        /* Locator enabled */
        LOCATOR,
        /* Networking enabled */
        NETWORK
    } state;
    Key key;
    ImVec2 inner_size;
    bool login_state;
    const char* error;
};
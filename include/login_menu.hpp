#pragma once

struct Context;

#include "imgui.h"
#include "locator.hpp"

struct Login_Menu {
public: 
    Login_Menu(Context* context);
    void draw();

private:
    void draw_inner();
    void draw_key();
    void draw_enter();

    Key key;
    ImVec2 inner_size;
    Context* ctx;
};
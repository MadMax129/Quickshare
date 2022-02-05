#pragma once

#include "imgui.h"
#include "networking.h"
#include <vector>
#include <memory>

struct Context;

struct Users_Menu {
    Users_Menu(Context* context);
    ~Users_Menu();

    void draw();
    void tests();

private:
    void update_list();

    Context* ctx;
    std::vector<Tcp_Msg::Id> users;
    std::unique_ptr<Tcp_Msg> msg_buf;
    ImGuiTextFilter filter;
    bool state; //true if in global user list | false when in friends
};

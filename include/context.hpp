#pragma once

#include <GLFW/glfw3.h>
#include "imgui.h"
#include "main_menu.hpp"
#include "login_menu.hpp"
#include "network.hpp"
#include <memory>

struct Context {
public:
    enum App_State {
        S_ERROR,
        S_LOGIN,
        S_MAIN_MENU
    };

    Context();
    bool create_window(int width, int height, const char* name);
    void init_imgui();
    void main_loop();
    void menu_bar();
    inline void set_appstate(App_State state) { app_state = state; }

    Locator loc;
    Network net;

private:
    void error_window();
    void init_style();
    void render();

    Main_Menu f_menu;
    Login_Menu l_menu;

    const char* error;
    App_State app_state;
    GLFWwindow* window;
    char* glsl_version;
    ImVec4 clear_color;
};
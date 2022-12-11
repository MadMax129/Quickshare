#pragma once

#include <GLFW/glfw3.h>
#include "imgui.h"
#include "main_menu.hpp"
#include "login_menu.hpp"
#include "network.hpp"
#include "state.hpp"
#include "thread_manager.hpp"
#include <memory>

struct Context {
public:
    enum State {
        ERROR_WINDOW,
        LOGIN,
        MAIN_MENU,
        CLOSE
    };

    Context();

    bool create_window(int width, int height, const char* name);
    void init_imgui();
    void main_loop();

    void set_appstate(State state);
    inline State get_appstate() { return app_state.get(); }

    Locator loc;
    Network net;

private:
    void menu_bar();
    void error_window();
    void init_style();
    void render();
    void cleanup();
    bool load_icon();

    Main_Menu f_menu;
    Login_Menu l_menu;

    const char* error;
    State_Manager<State> app_state;
    GLFWwindow* window;
    char* glsl_version;
    ImVec4 clear_color;
};
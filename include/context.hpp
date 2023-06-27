#pragma once

#include <GLFW/glfw3.h>
#include "imgui.h"
#include "main_menu.hpp"
#include "login_menu.hpp"
#include "network.hpp"
#include "state.hpp"
#include "thread_manager.hpp"
#include <memory>
#include "share_manager.hpp"

#define WINDOW_HEIGHT 600
#define WINDOW_WIDTH 600
#define ICON_PATH "../images/logo.png"
#define FONT_SIZE 15.0f

class Context {
public:
    enum State {
        ERROR_WINDOW,
        LOGIN,
        MAIN_MENU,
        CLOSE
    };

    static Context& get_instance() {
        static Context instance;
        return instance;
    }

    bool create_window(int width, int height, const char* name);
    void init_imgui();
    void main_loop();

    void set_appstate(State state);
    inline State get_state() { 
        return app_state.get(); 
    }

private:
    Context();
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
    
    void menu_bar();
    void error_window();
    void init_style();
    void render();
    void cleanup();
    bool load_icon();

    Main_Menu f_menu;
    Login_Menu l_menu;

    GLFWwindow* window;
    const char* glsl_version;
    const char* error;
    State_Manager<State> app_state;
    char pc_name[PC_NAME_MAX_LEN];
    ImVec4 clear_color;
};
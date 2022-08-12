#pragma once

#include "imgui.h"
#include <GLFW/glfw3.h>
#include "main_menu.hpp"
#include <memory>
#include "network.hpp"

#define WINDOW_HEIGHT 500
#define WINDOW_WIDTH 300

#define ICON_PATH "../images/logo.png"
#define FONT_SIZE 14.0f

struct Context {
public:
    enum App_State {
        ERROR_WINDOW,
        LOGIN,
        MAIN_MENU
    };
    Context(Network* network);
    ~Context(); 
    bool create_window(int width, int height, const char* name);
    void init_imgui();
    void main_loop();
    void menu_bar();

    Main_Menu f_menu;
    Network* network;

private:
    void error_window();
    void init_style();

    App_State app_state;
    GLFWwindow* window;
    char* glsl_version;
    ImVec4 clear_color;
};

#define IMGUI_NEW_FRAME() \
    ImGui_ImplOpenGL3_NewFrame(); \
    ImGui_ImplGlfw_NewFrame(); \
    ImGui::NewFrame();

#define IMGUI_CLEANUP() \
    ImGui_ImplOpenGL3_Shutdown(); \
    ImGui_ImplGlfw_Shutdown(); \
    ImGui::DestroyContext();

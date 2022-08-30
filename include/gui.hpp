#pragma once

#include "imgui.h"
#include <GLFW/glfw3.h>
#include "main_menu.hpp"
#include <memory>

struct Context {
public:
    enum App_State {
        ERROR_WINDOW,
        LOGIN,
        MAIN_MENU
    };

    Context();
    bool create_window(int width, int height, const char* name);
    void init_imgui();
    void main_loop();
    void menu_bar();

private:
    void error_window();
    void init_style();

    Main_Menu f_menu;
    const char* error;
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

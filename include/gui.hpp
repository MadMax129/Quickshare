#pragma once

#include "imgui.h"
#include <GLFW/glfw3.h>
#include "main_menu.hpp"
#include "login_menu.hpp"
#include "locator.hpp"
#include <memory>

struct Context {
public:
    enum App_State {
        S_ERROR,
        S_LOGIN,
        S_MAIN_MENU
    };

    Context(Locator& loc);
    bool create_window(int width, int height, const char* name);
    void init_imgui();
    void main_loop();
    void menu_bar();

    Locator& loc;
private:
    void error_window();
    void init_style();

    Main_Menu f_menu;
    Login_Menu l_menu;

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

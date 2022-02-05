#pragma once

#include "imgui.h"
#include <GLFW/glfw3.h>
#include "networking.h"
#include "chat_menu.h"
#include "file_menu.h"
#include "login_menu.h"
#include "user_list_menu.h"
#include <memory>

#define ICON_PATH "C:\\Program Files\\Quickshare\\icon\\logo.png"
#define FONT_SIZE 14.0f

struct Context {
public:
    enum App_State {
        ERROR_WINDOW,
        LOGIN,
        MAIN_MENU
    };
    Context(Client_Sock* client);
    ~Context(); 
    bool create_window(int width, int height, const char* name);
    void init_imgui();
    void main_loop();
    void menu_bar();
    void change_state(App_State state);

    Client_Sock* clisock;

    Login_Menu l_menu;
    Users_Menu u_menu;
    Chat_Menu  c_menu;
    File_Menu f_menu;

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

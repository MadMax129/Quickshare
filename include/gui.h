#pragma once

#include "imgui.h"
#include <GLFW/glfw3.h>
#include <memory.h>
#include <vector>
#include "networking.h"
#include "quickshare.h"

#define FONT_PATH ".././lib/imgui-1.85/misc/fonts/Roboto-Medium.ttf"
#define FONT_SIZE 14.0f
#define ICON_PATH "../images/logo.png"

struct Context;

struct Login_Menu {
    enum {
        L_DEFAULT,
        L_CLICKED_ENTER,
        L_CONNCETING,
        L_FAILED_TO_CONNECT,
        L_CONNECTED
    } local_state;
    bool started_connection;

    Login_Menu(Context* context);
    void draw();
    void reset();

private:
    Context* ctx;
    char username[USERNAME_MAX_LIMIT];
};

struct Chat_Menu {
    Chat_Menu(Context* context);
    ~Chat_Menu();
    
    void test();
    void draw();

private:
    void update_msgs();

    Context* ctx;
    std::vector<Tcp_Msg::Chat_Msg> msgs;
    Tcp_Msg* buf;
    static const unsigned int MAX_MSG_AMT = 60;
};

struct Users_Menu {
    Users_Menu(Context* context);
    ~Users_Menu();

    void draw();
    void tests();


private:
    void update_list();

    Context* ctx;
    std::vector<Tcp_Msg::Id> users;
    Tcp_Msg* buf;
    ImGuiTextFilter filter;
};

struct File_Menu {
    File_Menu(Context* context);
    ~File_Menu();

    void draw();
    void set_state(bool state);
    bool get_state() const;

private:
    bool open;
    Context* ctx;
};

struct Context {
public:
    enum App_State {
        S_ERROR,
        S_REGISTER,
        S_MAIN_MENU
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
    App_State app_state;
    GLFWwindow* window;
    char* glsl_version;
    ImVec4 clear_color;

    void error_window();
};


#define IMGUI_NEW_FRAME() \
    ImGui_ImplOpenGL3_NewFrame(); \
    ImGui_ImplGlfw_NewFrame(); \
    ImGui::NewFrame();

#define IMGUI_CLEANUP() \
    ImGui_ImplOpenGL3_Shutdown(); \
    ImGui_ImplGlfw_Shutdown(); \
    ImGui::DestroyContext();

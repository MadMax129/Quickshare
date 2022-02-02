#pragma once

#include "imgui.h"
#include <GLFW/glfw3.h>
#include <vector>
#include "networking.h"
#include <memory>

#define ICON_PATH "C:\\Program Files\\Quickshare\\icon\\logo.png"
#define FONT_SIZE 14.0f

struct Context;

struct Login_Menu {
    Login_Menu(Context* context);
    void draw();
    void reset();

private:
    enum {
        L_DEFAULT,
        L_CONNCETING,
        L_FAILED,
        L_CONNECTED
    } sign_in_state, sign_up_state;

    Context* ctx;
    char username[USERNAME_MAX_LIMIT];
    char password[PASSWORD_MAX_LIMIT];
    bool sign_up;
    
    void draw_sign_up();
    inline bool fields_empty(); 
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
    std::unique_ptr<Tcp_Msg> msg_buf;
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
    std::unique_ptr<Tcp_Msg> msg_buf;
    ImGuiTextFilter filter;
    boolean state; //true if in global user list | false when in friends
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

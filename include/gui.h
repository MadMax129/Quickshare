#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <GLFW/glfw3.h>
#include <memory.h>
#include "networking.h"
#include "quickshare.h"

#define FONT_PATH ".././lib/imgui-1.85/misc/fonts/Roboto-Medium.ttf"
#define FONT_SIZE 14.0f
// struct User {
//     // add id and username
// };

struct Context {
public:
    Context(ClientSock* client);
    ~Context(); 
    bool create_window(int width, int height, const char* name);
    void init_imgui();
    void main_loop();

    enum State {
        S_REGISTER,
        S_MAIN_MENU
    };

private:
    State state;
    GLFWwindow* window;
    char* glsl_version;
    ImVec4 clear_color;
    char username[USERNAME_MAX_LIMIT];
    Chat_Msg** msg_array;
    unsigned int msg_count;

    ClientSock* clisock;

    void login_menu();
    void main_menu();
    void chat_menu();
};

#define IMGUI_NEW_FRAME() \
    ImGui_ImplOpenGL3_NewFrame(); \
    ImGui_ImplGlfw_NewFrame(); \
    ImGui::NewFrame();

#define IMGUI_CLEANUP() \
    ImGui_ImplOpenGL3_Shutdown(); \
    ImGui_ImplGlfw_Shutdown(); \
    ImGui::DestroyContext();

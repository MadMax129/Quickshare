#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <GLFW/glfw3.h> 
#include "gui.h"
#include "networking.h"

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

Context::Context(Client_Sock* client) : l_menu(this), u_menu(this), c_menu(this), f_menu(this)
{
    window = NULL;
    glsl_version = NULL;
    app_state = S_REGISTER;
    clisock = client;
}

Context::~Context() {}

bool Context::create_window(int width, int height, const char* name) 
{
    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit())
        return false;

    glsl_version = (char*)"#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    window = glfwCreateWindow(width, height, name, NULL, NULL);

    if (window == NULL)
        return false;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    
    return true;
}

void Context::init_imgui() 
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // io.IniFilename = 
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 4.0f; 
    ImGui_ImplGlfw_InitForOpenGL(this->window, true);
    ImGui_ImplOpenGL3_Init(this->glsl_version);
    io.Fonts->AddFontFromFileTTF(FONT_PATH, FONT_SIZE);
    clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
}

void Context::error_window()
{
    ImGui::Begin("Error", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    ImGui::SetWindowSize(ImVec2(300,100));

    ImGui::TextColored(ImVec4(1.0f, 0.0, 0.0, 1.0f), "Error occured with connection to server...");
    
    // Attempt to reconnect to server
    if (ImGui::Button("Return")) {
        // Check if socket connection can be opened, continue back to login menu
        if (clisock->init_socket()) {
            change_state(S_REGISTER);
            clisock->connected = -1;
        }
    }

    ImGui::End();
}

void Context::menu_bar()
{
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Settings")) {
            if (ImGui::BeginMenu("Background Color")) {
                ImGui::ColorEdit3("MyColor##1", (float*)&clear_color);
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        else if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("File Sharing")) {
                f_menu.open = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void Context::change_state(App_State state)
{
    // When changing state reset menu parameters
    assert(app_state != state);
    if (state == S_REGISTER)
        l_menu.reset();

    app_state = state;
}

void Context::main_loop() 
{
    u_menu.tests();
    c_menu.test();
    app_state = S_MAIN_MENU;
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        IMGUI_NEW_FRAME();

        menu_bar();

        // Check if connection was lost with server
        if (app_state == S_MAIN_MENU && clisock->connected == 0) 
            change_state(S_ERROR);

        switch (app_state)
        {
            case S_ERROR:
                error_window();
                break;

            case S_REGISTER:
                l_menu.draw();
                break;

            case S_MAIN_MENU:
                u_menu.draw();
                c_menu.draw();
                if (f_menu.open){
                    f_menu.draw();
                }
                break;
        }



        ImGui::ShowDemoWindow();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Cleanup
    IMGUI_CLEANUP();
    glfwDestroyWindow(window);
    glfwTerminate();
}

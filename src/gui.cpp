#include <stdio.h>
#include <GLFW/glfw3.h> 
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "gui.h"
#include "quickshare.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../lib/stb_image.h"

#include "fonts/roboto-medium_font.h"

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

Context::Context(Client_Sock* client) : l_menu(this), u_menu(this), 
                                        c_menu(this), f_menu(this)
{
    window = NULL;
    glsl_version = NULL;
    app_state = LOGIN;
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

    GLFWimage images;
    images.width = 630;
    images.height = 630;

    images.pixels = stbi_load(ICON_PATH, &images.width, &images.height, 0, 4);
    if (!images.pixels)
        FATAL("Failed to load icon '%s'\n", ICON_PATH);
    glfwSetWindowIcon(window, 1, &images); 
    stbi_image_free(images.pixels);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    
    return true;
}

void Context::init_imgui() 
{
    // Init ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // init_style();

    // Glfw Backend setup
    ImGui_ImplGlfw_InitForOpenGL(this->window, true);
    ImGui_ImplOpenGL3_Init(this->glsl_version);
    
    ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(font_tff_data_compressed_data_base85, FONT_SIZE);
    
    // Default Background color
    clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
}

void Context::error_window()
{
    ImGui::Begin("Error", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    ImGui::SetWindowSize(ImVec2(300,100));

    ImGui::TextColored(ImVec4(1.0f, 0.0, 0.0, 1.0f), "Error occured with connection to server...");
    
    // Attempt to reconnect to server
    if (ImGui::Button("Return")) 
        change_state(LOGIN);

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
        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("File Sharing")) {
                f_menu.set_state(true);
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
    if (state == LOGIN)
        l_menu.reset();

    app_state = state;
}

void Context::main_loop() 
{
    // u_menu.tests();
    c_menu.test();
    app_state = MAIN_MENU;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        IMGUI_NEW_FRAME();

        menu_bar();

        
        // Check if connection was lost with server
        // Comment out for MAIN_MENU Testing
        // if (app_state == MAIN_MENU &&
        //     clisock->get_state() == Client_Sock::State::FAILED) 
        //     change_state(ERROR_WINDOW);

        switch (app_state)
        {
            case ERROR_WINDOW:
                error_window();
                break;

            case LOGIN:
                l_menu.draw();
                break;

            case MAIN_MENU:
                u_menu.draw();
                c_menu.draw();
                if (f_menu.get_state())
                    f_menu.draw();
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

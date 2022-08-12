#include <stdio.h>
#include <GLFW/glfw3.h> 
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "gui.hpp"
#include "quickshare.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "../lib/stb_image.h"
#include "roboto-medium_font.h"

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

Context::Context(Network* network) : f_menu(this)
{
    window = NULL;
    glsl_version = NULL;
    app_state = LOGIN;
    this->network = network;
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
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    
    window = glfwCreateWindow(width, height, name, NULL, NULL);

    if (window == NULL)
        return false;

    GLFWimage images;
    images.width = 630;
    images.height = 630;

    images.pixels = stbi_load(ICON_PATH, &images.width, &images.height, 0, 4);
    if (!images.pixels) {
        P_ERRORF("Failed to load icon '%s'\n", ICON_PATH);
        return false;
    }
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
    if (ImGui::Button("Return")) ;

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
        ImGui::EndMainMenuBar();
    }
}

void Context::main_loop() 
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        IMGUI_NEW_FRAME();

        menu_bar();

        f_menu.draw();

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

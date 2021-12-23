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

Context::Context(ClientSock* client)
{
    window = NULL;
    glsl_version = NULL;
    state = S_REGISTER;
    clisock = client;
    memset(username, 0, USERNAME_MAX_LIMIT);
    // msg_array = new Chat_Msg*[64];
    // msg_count = 0;
}

Context::~Context()
{
    // delete msg_array;
}

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
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(this->window, true);
    ImGui_ImplOpenGL3_Init(this->glsl_version);
    io.Fonts->AddFontFromFileTTF(FONT_PATH, FONT_SIZE);
    clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // chat = Chat();
}

void Context::login_menu() 
{
    ImGui::Begin("Login", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    ImGui::SetWindowSize(ImVec2(300, 200));
    // ImGui::SetNextWindowSize(ImVec2(300, 200));

    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Welcome to Quickshare!");
    ImGui::SameLine();
    ImGui::Text("Please enter a username.");

    char anim[11];
    sprintf(anim, "%c###ufield", "|/-\\"[(int)(ImGui::GetTime() / 0.25f) & 3]);
    ImGui::InputText(anim, username, USERNAME_MAX_LIMIT);

    if (ImGui::Button("Enter")) 
    {
        if (strlen(username) != 0) {
            if (!clisock->send_intro(username)) {
                LOGGER("FAILED TO SEND INTRO %s", username);
            }
            else {
                state = S_MAIN_MENU;
                clisock->start_recv();
            }
        }
    }

    ImGui::End();
}

void Context::chat_menu() 
{
    ImGui::Begin("Global Chat", NULL);
    char input[10] = {0};

    ImGui::BeginChild("Log", ImVec2(0, ImGui::GetWindowHeight() - 85), true);

    static bool got = false;
    static Tcp_Msg* m;
    if (clisock->msg_queue.size() > 0)
       m = clisock->msg_queue.pop(), got = true;

    // ImGui::Text(text);
    if (got) {
        ImGui::TextColored(ImVec4(0.77, .188, .201, 1.0), "%s:", (const char*)m->data.chat.from_user); 
        ImGui::SameLine();
        ImGui::TextWrapped((const char*)m->data.chat.data);
    }

    ImGui::EndChild();

    if (ImGui::InputText("Input", input, 10,  ImGuiInputTextFlags_EnterReturnsTrue)) {
        printf("Done '%s'\n", input);
    }

    ImGui::End();
}

void Context::main_menu() 
{
    ImGui::Begin("Contacts", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    ImVec2 outer_size = ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 10);
    if (ImGui::BeginTable("split2", 1, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_ScrollY, outer_size))
    {
        for (int i = 0; i < 100; i++)
        {
            ImGui::TableNextColumn();
            if (ImGui::TreeNode((void*)(intptr_t)i, "Contact %d", i))
            {
                ImGui::Text("Status: %s", "Active");
                ImGui::SameLine();
                if (ImGui::SmallButton("button")) {}
                ImGui::TreePop();
            }
                
        }
        ImGui::EndTable();
    }
    // ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
}

void Context::main_loop() 
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        IMGUI_NEW_FRAME();

        switch (state)
        {
            case S_REGISTER: 
                login_menu();
                break;

            case S_MAIN_MENU:
                main_menu();
                chat_menu();
                break;
        }

        ImGui::ShowDemoWindow();
        // ImGuiStyle& style = ImGui::GetStyle();
        // style.FrameRounding = 4.0f;

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

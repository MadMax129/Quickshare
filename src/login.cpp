#include <stdio.h>
#include "gui.h"
#include "networking.h"

Login_Menu::Login_Menu(Context* context)
{
    ctx = context;
    local_state = L_DEFAULT;
    started_connection = false;
    memset(username, 0, USERNAME_MAX_LIMIT);
}

void Login_Menu::reset()
{
    local_state = L_DEFAULT;
    started_connection = false;
    memset(username, 0, USERNAME_MAX_LIMIT);
}

void Login_Menu::draw()
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

    if (ImGui::Button("Enter")) {
        if (local_state != L_CONNCETING)
            local_state = L_CLICKED_ENTER;
    }

    switch (local_state)
    {
        case L_CLICKED_ENTER: {
            if (strlen(username) != 0)
                local_state = L_CONNCETING;
            else
                local_state = L_DEFAULT;
            break;
        }

        case L_CONNCETING: {
            if (!started_connection) {
                ctx->clisock->start_connection();
                started_connection = true;
            }

            if (ctx->clisock->has_connected() == -1) {
                ImGui::NewLine();
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Connecting...");
            }
            else if (ctx->clisock->has_connected() == 0) {
                local_state = L_FAILED_TO_CONNECT;
                started_connection = false;
            }
            else {
                local_state = L_CONNECTED;
            }

            break;
        }

        case L_FAILED_TO_CONNECT: {
            ImGui::NewLine();
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to connect...");
            break;
        }

        case L_CONNECTED: {
            if (!ctx->clisock->send_intro(username)) {
                local_state = L_FAILED_TO_CONNECT;
            }
            else {
                ctx->clisock->start_recv();
                ctx->change_state(ctx->S_MAIN_MENU);
            }
            break;
        }
        
        default: break;
    }

    ImGui::End();
}
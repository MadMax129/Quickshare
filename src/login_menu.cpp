#include <stdio.h>
#include "gui.h"

#define SIGN_IN_TEMPLATE "Login %c###sign_in"
#define SIGN_UP_TEMPLATE "%c###sign_up"

Login_Menu::Login_Menu(Context* context)
{
    ctx = context;
    reset();
}

void Login_Menu::reset()
{
    sign_in_state = L_DEFAULT;
    sign_up_state = L_DEFAULT;
    sign_up = false;
    memset(username, 0, USERNAME_MAX_LIMIT);
    memset(password, 0, PASSWORD_MAX_LIMIT);
}

void Login_Menu::draw()
{
    char anim[sizeof(SIGN_IN_TEMPLATE) + 1];
    sprintf(anim, SIGN_IN_TEMPLATE, "|/-\\"[(int)(ImGui::GetTime() / 0.5f) & 3]);
    ImGui::Begin(anim, NULL, ImGuiWindowFlags_NoCollapse);

    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Welcome to Quickshare!");
    ImGui::NewLine();

    // Get username
    ImGui::Text("Enter your username");
    ImGui::InputText("##ufield", username, USERNAME_MAX_LIMIT);

    // Get password
    ImGui::Text("Enter your password");
    ImGui::InputText("##pfield", password, PASSWORD_MAX_LIMIT, ImGuiInputTextFlags_Password);

    // ImGui::NewLine();
    if (ImGui::Button("Sign In", ImVec2(100.0f, 20.0f))) {
        if (!fields_empty())
            sign_in_state = L_CONNCETING;
    }
    if (ImGui::Button("Sign Up", ImVec2(100.0f, 20.0f)))
        sign_up = true;

    switch (sign_in_state)
    {
        case L_CONNCETING: {
            if (ctx->clisock->get_state() == Client_Sock::State::FAILED) {
                sign_in_state = L_FAILED;
            }
            else {
                sign_in_state = L_CONNECTED;
            }

            break;
        }

        case L_FAILED: {
            ImGui::NewLine();
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed, please try again...");
            break;
        }

        case L_CONNECTED: {
            if (!ctx->clisock->send_intro(username)) {
                sign_in_state = L_FAILED;
            }
            else {
                ctx->change_state(ctx->MAIN_MENU);
            }
            break;
        }
        
        default: break;
    }

    ImGui::End();

    if (sign_up)
        draw_sign_up();
}

void Login_Menu::draw_sign_up()
{
    ImGui::Begin("Sign Up", &sign_up, ImGuiWindowFlags_NoCollapse);

    // Get username
    ImGui::Text("Enter a username");
    ImGui::InputText("##usfield", username, USERNAME_MAX_LIMIT);

    // Get password
    ImGui::Text("Enter a password");
    ImGui::InputText("##psfield", password, PASSWORD_MAX_LIMIT, ImGuiInputTextFlags_Password);

    if (ImGui::Button("Enter")) {
        if (!fields_empty())
            sign_up_state = L_CONNCETING;
    }

    switch (sign_up_state) 
    {
        case L_CONNCETING: {

            break;
        }

        default: break;
    }

    ImGui::End();
}

inline bool Login_Menu::fields_empty()
{
    if (strnlen(username, USERNAME_MAX_LIMIT) == 0 || 
        strnlen(password, PASSWORD_MAX_LIMIT) == 0)
        return true;
    return false;
} 
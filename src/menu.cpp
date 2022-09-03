#include "gui.hpp"

#include <iostream>
#include <windows.h>
#include <vector>

#include <d3d9.h>

bool doOnce = false;

bool show_login = true;
bool show_register = false;

class initWindow {
public:
    const char* window_title = "Loader";
    ImVec2 window_size{ 740, 460 };
    
    DWORD window_flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;
} iw;

void load_styles()
{
    ImVec4* colors = ImGui::GetStyle().Colors;
    {
        colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);

        colors[ImGuiCol_FrameBg] = ImColor(11, 11, 11, 255);
        colors[ImGuiCol_FrameBgHovered] = ImColor(11, 11, 11, 255);

        colors[ImGuiCol_Button] = ImColor(255, 0, 46, globals.button_opacity);
        colors[ImGuiCol_ButtonActive] = ImColor(255, 0, 46, globals.button_opacity);
        colors[ImGuiCol_ButtonHovered] = ImColor(255, 0, 46, globals.button_opacity);

        colors[ImGuiCol_TextDisabled] = ImVec4(0.37f, 0.37f, 0.37f, 1.00f);
    }

    ImGuiStyle* style = &ImGui::GetStyle();
    {
        style->WindowPadding = ImVec2(4, 4);
        style->WindowBorderSize = 0.f;

        style->FramePadding = ImVec2(8, 6);
        style->FrameRounding = 3.f;
        style->FrameBorderSize = 1.f;
    }
}

void render()
{

    if (globals.active)
    {
        if (!doOnce)
        {
            load_styles();
            doOnce = true;
        }

        ImGui::SetNextWindowSize(iw.window_size);

        ImGui::Begin(iw.window_title, &globals.active, iw.window_flags);
        {
            ImGui::SetCursorPos(ImVec2(726, 5));
            ImGui::TextDisabled("X");
            if (ImGui::IsItemClicked())
            {
                globals.active = false;
            }

            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(27.0f/255.0f, 27.0f/255.0f, 27.0f/255.0f, 255));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(27.0f/255.0f, 27.0f/255.0f, 27.0f/255.0f, 255));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 3.f);
            {
                ImGui::SetCursorPos(ImVec2(222, 83));
                ImGui::BeginChild("##MainPanel", ImVec2(300, 276), true);
                {
                    if (show_login)
                    {
                        ImGui::SetCursorPos(ImVec2(90, 20));
                        ImGui::TextDisabled("Welcome to Quickshare");

                        ImGui::SetCursorPos(ImVec2(97, 35));
                        ImGui::Text("Enter a session key");

                        ImGui::PushItemWidth(260.f);
                        {
                            ImGui::SetCursorPos(ImVec2(22, 79));
                            ImGui::TextDisabled("Key");

                            ImGui::SetCursorPos(ImVec2(20, 95));
                            ImGui::InputText("##Username", globals.user_name, IM_ARRAYSIZE(globals.user_name));
                        }
                        ImGui::PopItemWidth();

                        ImGui::SetCursorPos(ImVec2(22, 190));
                        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.f);
                        if (ImGui::Button("Login", ImVec2(260.f, 30.f)))
                        {
                            show_login = false;
                            show_register = true;
                        }
                        ImGui::PopStyleVar();

                        ImGui::SetCursorPos(ImVec2(22, 240));
                        ImGui::TextDisabled("Don't have a key? Create one!");
                    }


                    if (show_register)
                    {
                        ImGui::SetCursorPos(ImVec2(118, 20));
                        ImGui::TextDisabled("Welcome Back");

                        ImGui::SetCursorPos(ImVec2(95, 35));
                        ImGui::Text("Register For An Account");

                        ImGui::PushItemWidth(260.f);
                        {
                            ImGui::SetCursorPos(ImVec2(22, 79));
                            ImGui::TextDisabled("Username");

                            ImGui::SetCursorPos(ImVec2(20, 95));
                            ImGui::InputText("##Username", globals.user_name, IM_ARRAYSIZE(globals.user_name));
                        }
                        ImGui::PopItemWidth();

                        ImGui::PushItemWidth(260.f);
                        {
                            ImGui::SetCursorPos(ImVec2(22, 130));
                            ImGui::TextDisabled("Invite Key");

                            ImGui::SetCursorPos(ImVec2(20, 146));
                            ImGui::InputText("##InviteKey", globals.invite_key, IM_ARRAYSIZE(globals.invite_key));
                        }
                        ImGui::PopItemWidth();

                        ImGui::SetCursorPos(ImVec2(22, 190));
                        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.f);
                        if (ImGui::Button("Register", ImVec2(260.f, 30.f)))
                        {
                            show_login = true;
                            show_register = false;
                        }
                        ImGui::PopStyleVar();

                        ImGui::SetCursorPos(ImVec2(22, 240));
                        ImGui::TextDisabled("Already have an account? Sign in now!");
                    }

                }
                ImGui::EndChild();
            }
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(1);

        }
        ImGui::End();
    }
    else
    {
        exit(0);
    }
}
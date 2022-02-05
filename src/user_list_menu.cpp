#include <stdio.h>
#include "gui.h"
#include <algorithm>
#include <cstring>
#include "quickshare.h"

Users_Menu::Users_Menu(Context* context) : msg_buf(new Tcp_Msg)
{
    ctx = context;
}

void Users_Menu::tests() 
{
    Tcp_Msg msg;
    msg.m_type = Msg_Type::USER_ADD;
    std::strcpy((char*)msg.id.username, "Maks");
    ctx->clisock->msg_queue.push(&msg);
    std::strcpy((char*)msg.id.username, "Weyne");
    ctx->clisock->msg_queue.push(&msg); 
    std::strcpy((char*)msg.id.username, "Shawn");
    ctx->clisock->msg_queue.push(&msg); 
}

Users_Menu::~Users_Menu() {}

void Users_Menu::update_list()
{
    ctx->clisock->msg_queue.pop(msg_buf.get());
    
    if (msg_buf->m_type == Msg_Type::USER_ADD) {
        users.push_back(msg_buf->id); 
    }
    else {
        int i = 0;
        for (const auto &e : users) {
            if (std::strncmp(
                reinterpret_cast<const char*>(e.username), 
                reinterpret_cast<const char*>(msg_buf->id.username), 
                USERNAME_MAX_LIMIT) == 0) {
                users.erase(users.begin()+i);
                break;
            }
            i++;
        }
    }

    // Sort alphabetically
    std::sort(users.begin(), users.end(), [](auto& str1, auto& str2) {
        return std::strncmp(
            reinterpret_cast<const char*>(str1.username), 
            reinterpret_cast<const char*>(str2.username), 
            USERNAME_MAX_LIMIT) < 0;
    });
}

void Users_Menu::draw()
{
    Msg_Type peek = ctx->clisock->msg_queue.peek();
    if (peek == Msg_Type::USER_ADD || peek == Msg_Type::USER_REMOVE)
        update_list();

    ImGui::Begin("User List", NULL);

    if (ImGui::BeginTabBar("User Tabs", ImGuiTabBarFlags_Reorderable)) {
        if(ImGui::BeginTabItem("Global Users")) {

            filter.Draw("Search");
            for (size_t i = 0; i < users.size(); i++) {
                if (filter.PassFilter((char*)users.at(i).username)) {
                    ImGui::Selectable((char*)users.at(i).username);
                    if(ImGui::BeginPopupContextItem()) {
                        // ImGui::Selectable("Share");
                        if (ImGui::Button("Share")) {
                            ctx->f_menu.set_state(true);
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::SameLine();
                        //coloring buttons
                        // ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(3 / 7.0f, 0.6f, 0.6f));
                        // ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(3 / 7.0f, 0.7f, 0.7f));
                        // ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(3 / 7.0f, 0.8f, 0.8f)); 
                        if (ImGui::Button("Add as friend")) {
                            //send req to server. 
                            ImGui::CloseCurrentPopup();
                        }

                        // ImGui::PopStyleColor(3);  
                        ImGui::EndPopup();
                }
                    
                }
            }
            ImGui::EndTabItem();
        }
        
        if(ImGui::BeginTabItem("Friends")) {
            
            ImGui::BeginChild("Friend List", ImVec2(ImGui::GetWindowWidth()/2, ImGui::GetWindowHeight() - 60),true);
            

            ImGui::EndChild();

            ImGui::SameLine();
            ImGui::SetCursorPosX( (ImGui::GetWindowWidth()/2.0) + 10.0);
            ImGui::BeginChild("Friend Requests", ImVec2( (ImGui::GetWindowWidth()/2.0) - 16.0, ImGui::GetWindowHeight()/2.0),true);
            
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    ImGui::End();
}
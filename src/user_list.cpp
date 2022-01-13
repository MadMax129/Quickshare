#include <stdio.h>
#include "gui.h"
#include "networking.h"

Users_Menu::Users_Menu(Context* context)
{
    ctx = context;
    buf = (Tcp_Msg*)malloc(sizeof(Tcp_Msg));
    if (!buf)
        FATAL_MEM();
    memset(buf, 0, sizeof(Tcp_Msg));
}

void Users_Menu::tests() 
{
    Tcp_Msg msg;
    msg.m_type = Msg_Type::USER_ADD;
    strcpy((char*)msg.id.username, "maks");
    ctx->clisock->msg_queue.push(&msg);
    strcpy((char*)msg.id.username, "weyne");
    ctx->clisock->msg_queue.push(&msg); 
}

Users_Menu::~Users_Menu() 
{
    free(buf);
}

void Users_Menu::update_list()
{
    ctx->clisock->msg_queue.pop(buf);

    if (buf->m_type == Msg_Type::USER_ADD) {
        printf("New client %s\n", buf->id.username);
        // users.push_back(buf->data.intro);
    }
    else {
        printf("Client dissconnected %s\n", buf->id.username);
        // for (const auto &e : users) {
        //     if (strcmp(e, buf->data.intro.username))
        //         printf("Erase %s\n", e);
        // }
    } 
}

void Users_Menu::draw()
{
    Msg_Type peek = ctx->clisock->msg_queue.peek();
    if (peek == Msg_Type::USER_ADD || peek == Msg_Type::USER_REMOVE)
        update_list();

    ImGui::Begin("User List", NULL, ImGuiWindowFlags_NoResize);
    ImGui::SetWindowSize(ImVec2(300, 300));
    ImVec2 outer_size = ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 10);
    if (ImGui::BeginTable("split2", 1, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_ScrollY, outer_size))
    {
        for (size_t i = 0; i < users.size(); i++)
        {
            ImGui::TableNextColumn();
            if (ImGui::TreeNode((void*)(intptr_t)i, "%s", users.at(i).username))
            {
                if (ImGui::SmallButton("Ping")) {}
                ImGui::TreePop();
            }
        }
        ImGui::EndTable();
    }
    ImGui::End();
}
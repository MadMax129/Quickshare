#include "gui.h"
#include "quickshare.h"
#include <cstring>

Chat_Menu::Chat_Menu(Context* context) : msg_buf(new Tcp_Msg)
{
    ctx = context;
    msgs.reserve(60);
}

Chat_Menu::~Chat_Menu() {}

void Chat_Menu::test()
{
    msg_buf->m_type = Msg_Type::GLOBAL_CHAT;
    for(size_t i = 0; i < 10; i++) {
        std::strcpy((char*)msg_buf->msg.data, "message I just found out, that need.");
        std::strcpy((char*)msg_buf->msg.username, "maks");
        ctx->clisock->msg_queue.push(msg_buf.get());
    }
}

void Chat_Menu::update_msgs()
{
    // use MyInputTextMultiline see (resize callbacks) and flags to make it read only
    ctx->clisock->msg_queue.pop(msg_buf.get());

    if (msg_buf->m_type == Msg_Type::GLOBAL_CHAT && msgs.size() < MAX_MSG_AMT) {
        msgs.push_back(msg_buf->msg);
    }
    else {
       msgs.erase(msgs.begin(), msgs.begin()+1);
       msgs.push_back(msg_buf->msg);
    }
}

void Chat_Menu::draw()
{
    Msg_Type peek = ctx->clisock->msg_queue.peek();
    if (peek == Msg_Type::GLOBAL_CHAT)
        update_msgs();

    ImGui::Begin("Global Chat", NULL);

    static char input[10] = {0};

    ImGui::BeginChild("Log", ImVec2(0, ImGui::GetWindowHeight() - 85), true, ImGuiWindowFlags_NoMove);

    for (const auto &e : msgs) {
        ImGui::TextColored(ImVec4(0.77, .188, .201, 1.0), "%s:", (char*)e.username); 
        ImGui::SameLine();
        ImGui::TextWrapped((char*)e.data);    
    }
    
    ImGui::EndChild();

    if (ImGui::InputText("Input", input, 10,  ImGuiInputTextFlags_EnterReturnsTrue)) {
        printf("Done '%s'\n", input);
    }

    ImGui::End();
}

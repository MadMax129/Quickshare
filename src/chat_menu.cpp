#include "gui.h"
#include "quickshare.h"

Chat_Menu::Chat_Menu(Context* context)
{
    ctx = context;
    msgs.reserve(60);

    buf = (Tcp_Msg*)malloc(sizeof(Tcp_Msg));
    if (!buf)
        FATAL_MEM();
    memset(buf, 0, sizeof(Tcp_Msg));
}

Chat_Menu::~Chat_Menu()
{
    free(buf);
}

void Chat_Menu::test()
{
    buf->m_type = Msg_Type::GLOBAL_CHAT;
    for(size_t i = 0; i < 10; i++) {
        strcpy((char*)buf->msg.data, "message I just found out, that need.");
        strcpy((char*)buf->msg.username, "maks");
        ctx->clisock->msg_queue.push(buf);
    }
}

void Chat_Menu::update_msgs()
{
    ctx->clisock->msg_queue.pop(buf);

    if (buf->m_type == Msg_Type::GLOBAL_CHAT && msgs.size() < MAX_MSG_AMT) {
        msgs.push_back(buf->msg);
    }
    else {
       msgs.erase(msgs.begin(), msgs.begin()+1);
       msgs.push_back(buf->msg);
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

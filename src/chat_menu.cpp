#include "gui.h"

Chat_Menu::Chat_Menu(Context* context)
{
    ctx = context;
    msgs.reserve(60);
}

Chat_Menu::~Chat_Menu() {}

void Chat_Menu::update_msgs()
{

}

void Chat_Menu::draw()
{
    Msg_Type peek = ctx->clisock->msg_queue.peek();
    if (peek == Msg_Type::GLOBAL_CHAT)
        update_msgs();

    ImGui::Begin("Global Chat", NULL);
    static char input[10] = {0};

    ImGui::BeginChild("Log", ImVec2(0, ImGui::GetWindowHeight() - 85), true, ImGuiWindowFlags_NoMove);
    ImGui::TextColored(ImVec4(0.77, .188, .201, 1.0), "Weyne:"); ImGui::SameLine();
    ImGui::TextWrapped("#include <iostream> #include <ctime> using namespace std; int main() { time_t curr_time; curr_time = time(NULL); char *tm = ctime(&curr_time); cout << Today is :  << tm; return 0; }");      
    ImGui::EndChild();

    if (ImGui::InputText("Input", input, 10,  ImGuiInputTextFlags_EnterReturnsTrue)) {
        printf("Done '%s'\n", input);
    }

    ImGui::End();
}

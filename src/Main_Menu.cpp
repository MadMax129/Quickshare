#include "gui.hpp"
#include <array>
#include <windows.h>
#include <string>
#include "nfd.h"

Main_Menu::Main_Menu(Context* context)
{
    ctx = context;
    list.client_count = 0;
}

void Main_Menu::set_state(bool state) 
{
    open = state;
}

bool Main_Menu::get_state() const
{
    return open;
}

void Main_Menu::draw_path()
{
    ImGui::SetNextWindowPos(ImVec2(50,30));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.44706f, 0.53725f, 0.85490f, 1.0f)); 
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);

    if (ImGui::BeginChildFrame(100, ImVec2(200,26), ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
        const char *path = "Path...";
        if (ImGui::Button(path, ImVec2(200,20)))
        {
            (void)open_file();
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }

    ImGui::EndChildFrame();
}

void Main_Menu::draw_backlog()
{
    ImGui::SetNextWindowPos(ImVec2(25,225));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.32941f, 0.33333f, 0.42353f, 1.0f)); 
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);

    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | 
                                       ImGuiWindowFlags_NoResize   | 
                                       ImGuiWindowFlags_NoMove;

    if (ImGui::BeginChildFrame(101, BACKLOG_WIN_SIZE, flags))
    {
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();

        add_event(Incoming, "asdmddasdsadasdasdasaksdm ", "d8.dll");
        add_event(Sent, "asdmasdasdaksdm ", "tsb.png");
        add_event(Sent, "asdmaksdm ", "word.txt");
        add_event(Incoming, "asdadsdnas ndksdm ", "qs.exe");
        add_event(Sent, "asdmaksdm ", "vid.mp4");
    }

    ImGui::EndChildFrame();
}

void Main_Menu::draw_users()
{
    ImGui::SetNextWindowPos(ImVec2(155,225));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.32941f, 0.33333f, 0.42353f, 1.0f)); 
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);

    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | 
                                       ImGuiWindowFlags_NoResize   | 
                                       ImGuiWindowFlags_NoMove     | 
                                       ImGuiWindowFlags_NoScrollbar;

    if (ctx->network->gui_msg.front() != nullptr && 
        ctx->network->gui_msg.front()->type == Msg::CLIENT_LIST)
    {
        list = ctx->network->gui_msg.front()->list;
        ctx->network->gui_msg.pop();
    }

    if (ImGui::BeginChildFrame(102, USERS_WIN_SIZE, flags))
    {
        // Msg::Client_List clients =  {"DESKTOP-1238452", "DESKTOP-1256620", "DESKTOP-856234"};
        add_clients();

        ImGui::PopStyleColor();
        ImGui::PopStyleVar();

        ImGui::SetCursorPos(ImVec2(75,220));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV( 4 / 7.0f, 0.6f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV( 4 / 7.0f, 0.7f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(4 / 7.0f, 0.8f, 0.8f));
        if(ImGui::Button("Send"))
        {
            
        }
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();
    }

    ImGui::EndChildFrame();
}

void Main_Menu::draw_request()
{
    ImGui::SetNextWindowPos(ImVec2(50,90));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.32941f, 0.33333f, 0.42353f, 1.0f)); 
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);

    ImGui::SetCursorPos(ImVec2(100,50));
    ImGui::Text("Incoming Requests");

    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | 
                                       ImGuiWindowFlags_NoResize   | 
                                       ImGuiWindowFlags_NoMove     |
                                       ImGuiWindowFlags_NoScrollbar;

    if (ImGui::BeginChildFrame(103, REQUESTS_WIN_SIZE, flags))
    {
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        // incoming_request("Test file in order to test word wrapping st file in order to test word wrapping st file in order to test word wrapping st file in order to test word wrapping ", "kamdkasdkmaksdm.exe");
    }

    ImGui::EndChildFrame();
}

void Main_Menu::draw()
{
    ImGui::SetNextWindowPos(ImVec2(0, MENU_BAR_MARGIN));
    ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.7f));

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse  | 
                                   ImGuiWindowFlags_NoScrollbar | 
                                   ImGuiWindowFlags_NoResize    | 
                                   ImGuiWindowFlags_NoMove      |  
                                   ImGuiWindowFlags_NoTitleBar;

    if (ImGui::Begin("menu", &open, flags)) 
    {
        ImGui::PopStyleColor();
        // ImGui::GetStyle().WindowRounding = 0.0f; ???
        
        draw_path();
       
        // Window Titles
        ImGui::SetCursorPos(ImVec2(60,180));
        ImGui::Text("Backlog");
        ImGui::SameLine(180);
        ImGui::Text("Active Users");

        draw_backlog();
        draw_users();
        draw_request();

        ImGui::End();
    }
}

void Main_Menu::incoming_request(const char *desc, const char *fname )
{

    ImGui::SetCursorPosX(30); ImGui::SameLine(); ImGui::TextWrapped(fname);
    ImGui::SetCursorPosX(55);
    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0 / 7.0f, 0.6f, 0.6f));ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0 / 7.0f, 0.7f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0 / 7.0f, 0.8f, 0.8f));
    ImGui::Button("Deny");
    ImGui::PopStyleColor(3); 
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(2 / 7.0f, 0.6f, 0.6f));ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(2 / 7.0f, 0.7f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(2 / 7.0f, 0.8f, 0.8f));
    ImGui::Button("Accept");
    ImGui::PopStyleColor(3);
    ImGui::SetCursorPosX(70); more_info(desc); 
}

void Main_Menu::more_info(const char *desc)
{   
    static float wrap_width = 200.0f;
    if (ImGui::BeginPopupContextItem("my popup"))
    {
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + wrap_width);
        ImGui::Text(desc, wrap_width);
        ImGui::PopTextWrapPos();
        ImGui::EndPopup();
    }
    if(ImGui::Button("More Info"))
        ImGui::OpenPopup("my popup");
}


void Main_Menu::add_clients()
{
    static bool selected[3] { false, false, false };

    for (std::size_t i = 0; i < list.client_count; i++)
    {
        ImGui::Selectable(list.clients[i].name,  &selected[i]);
    }
}

void Main_Menu::add_event(Transfer_Type type, const char *desc, const char *fname)
{
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,0.5f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg,ImVec4(0.42941f, 0.43333f, 0.52353f, 1.0f));

    if (ImGui::BeginChildFrame(1, ImVec2(113,242), ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
    {
        switch(type)
        {
            case Incoming:
            {
                ImGui::Text("(?)");
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                    ImGui::Text(desc);
                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }
                ImGui::SameLine(0,5);
                ImGui::Text(fname);
                ImGui::SameLine(0,5);
                ImGui::TextColored(ImVec4(0.0178, 0.716, 0.890, 1.0f), "RECVD");   
                break;               
            }
            case Sent:
            {
                ImGui::Text("(?)");
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                    ImGui::Text(desc);
                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }
                ImGui::SameLine(0,5);
                ImGui::Text(fname);
                ImGui::SameLine(0,5);
                ImGui::TextColored(ImVec4(0.0178, 0.890, 0.105,1.0f), "SENT");
                break;
            }

            case Error:
            {
                break;
            }
        }
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
    }
    ImGui::EndChildFrame(); 
}

bool Main_Menu::open_file()
{
    nfdchar_t *outPath = NULL;
    nfdresult_t result = NFD_OpenDialog(NULL, NULL, &outPath);

    if (result == NFD_OKAY)
    {
        puts(outPath);
        free(outPath);
    }
    else if ( result == NFD_CANCEL )
    {
        puts("User pressed cancel.");
    }
    else 
    {
        printf("Error: %s\n", NFD_GetError() );
    }

    return true;
}
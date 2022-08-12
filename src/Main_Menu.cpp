#define NTDDI_VERSION 0x0A000006 //NTDDI_WIN10_RS5
#define _WIN32_WINNT 0x0A00 // _WIN32_WINNT_WIN10, the _WIN32_WINNT macro must also be defined when defining NTDDI_VERSION
#include "gui.h"
#include <array>
#include <windows.h>
#include <string>
#include <shobjidl.h> 

Main_Menu::Main_Menu(Context* context)
{
    ctx = context;
}

Main_Menu::~Main_Menu()
{

}

void Main_Menu::set_state(bool state) 
{
    open = state;
}

bool Main_Menu::get_state() const
{
    return open;
}

void Main_Menu::draw()
{   

    ImGui::SetNextWindowPos(ImVec2(0,20));
    ImGui::SetNextWindowSize(ImVec2(300, 500));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.7f));

    if (ImGui::Begin("title", &open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize 
                                    | ImGuiWindowFlags_NoMove |  ImGuiWindowFlags_NoTitleBar)) 
    {
        ImGui::PopStyleColor();
        ImGui::GetStyle().WindowRounding = 0.0f;

        ImGui::SetNextWindowPos(ImVec2(50,30));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.44706f, 0.53725f, 0.85490f, 1.0f)); 
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
        //Child window for Path button
        if (ImGui::BeginChildFrame(100, ImVec2(200,26), ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
            const char *path = "Path...";
            if (ImGui::Button(path, ImVec2(200,20)))
            {
                switch (openFile()) {
                    case(TRUE): {
                        printf("SELECTED FILE: %s\nFILE PATH: %s\n\n", sSelectedFile.c_str(), sFilePath.c_str());
                    }
                    case(FALSE): {
                        printf("ENCOUNTERED AN ERROR: (%d)\n", GetLastError());
                    }
                }
            }
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
        }
        //Window Titles
        ImGui::EndChildFrame();
        ImGui::SetCursorPos(ImVec2(60,180));
        ImGui::Text("Backlog");
        ImGui::SameLine(180);
        ImGui::Text("Active Users");

        //Backlog Child 
        ImGui::SetNextWindowPos(ImVec2(25,225));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.32941f, 0.33333f, 0.42353f, 1.0f)); 
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
        if (ImGui::BeginChildFrame(101, ImVec2(120,250), ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
        {
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
  
            addEvent(Incoming, "asdmddasdsadasdasdasaksdm ", "d8.dll");
            addEvent(Sent, "asdmasdasdaksdm ", "tsb.png");
            addEvent(Sent, "asdmaksdm ", "word.txt");
            addEvent(Incoming, "asdadsdnas ndksdm ", "qs.exe");
            addEvent(Sent, "asdmaksdm ", "vid.mp4");
            addEvent(Sent, "asdmakdasdasdassdm ", "test.py");
        }
        ImGui::EndChildFrame();

        //send window
        ImGui::SetNextWindowPos(ImVec2(155,225));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.32941f, 0.33333f, 0.42353f, 1.0f)); 
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);

        if (ImGui::BeginChildFrame(102, ImVec2(120,250), ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |ImGuiWindowFlags_NoScrollbar))
        {
            std::array<const char*, 3> clients =  {"DESKTOP-1238452", "DESKTOP-1256620", "DESKTOP-856234"};
            Main_Menu::addClients(clients);

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


        ImGui::SetNextWindowPos(ImVec2(50,90));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.32941f, 0.33333f, 0.42353f, 1.0f)); 
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);

        ImGui::SetCursorPos(ImVec2(100,50));
        ImGui::Text("Incoming Requests");
        if (ImGui::BeginChildFrame(103, ImVec2(200,100),ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |ImGuiWindowFlags_NoScrollbar ))
        {
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();

            incomingRequest("Test file in order to test word wrapping ", "test.exe");
        }
        ImGui::EndChildFrame();


        ImGui::End();
    }
}

void Main_Menu::incomingRequest(const char *desc, const char *fname )
{
    ImGui::Text("File Name:"); ImGui::SameLine(); ImGui::Text(fname);
    ImGui::Text("File Descprition:   ");ImGui::TextWrapped(desc);

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
    

}




void Main_Menu::addClients(std::array<const char*,3>&clientList)
{
    static bool selected[3] { false, false, false};

    for (int i = 0; i < clientList.size(); i++)
    {
        ImGui::Selectable(clientList[i],  &selected[i]);
        
    }
    
}

void Main_Menu::addEvent(Main_Menu::transferType type, const char *desc, const char *fname)
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
        }
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
    }
    ImGui::EndChildFrame(); 
}

bool Main_Menu::openFile()
{
    //  CREATE FILE OBJECT INSTANCE
    HRESULT f_SysHr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(f_SysHr))
        return FALSE;

    // CREATE FileOpenDialog OBJECT
    IFileOpenDialog *f_FileSystem;
    f_SysHr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&f_FileSystem));
    if (FAILED(f_SysHr)) {
        CoUninitialize();
        return FALSE;
    }

    //  SHOW OPEN FILE DIALOG WINDOW
    f_SysHr = f_FileSystem->Show(NULL);
    if (FAILED(f_SysHr)) {
        f_FileSystem->Release();
        CoUninitialize();
        return FALSE;
    }

    //  RETRIEVE FILE NAME FROM THE SELECTED ITEM
    IShellItem* f_Files;
    f_SysHr = f_FileSystem->GetResult(&f_Files);
    if (FAILED(f_SysHr)) {
        f_FileSystem->Release();
        CoUninitialize();
        return FALSE;
    }

    //  STORE AND CONVERT THE FILE NAME
    PWSTR f_Path;
    f_SysHr = f_Files->GetDisplayName(SIGDN_FILESYSPATH, &f_Path);
    if (FAILED(f_SysHr)) {
        f_Files->Release();
        f_FileSystem->Release();
        CoUninitialize();
        return FALSE;
    }

    //  FORMAT AND STORE THE FILE PATH
    std::wstring path(f_Path);
    std::string c(path.begin(), path.end());
    sFilePath = c;

    //  FORMAT STRING FOR EXECUTABLE NAME
    const size_t slash = sFilePath.find_last_of("/\\");
    sSelectedFile = sFilePath.substr(slash + 1);

    //  SUCCESS, CLEAN UP
    CoTaskMemFree(f_Path);
    f_Files->Release();
    f_FileSystem->Release();
    CoUninitialize();
    return TRUE;
}
 




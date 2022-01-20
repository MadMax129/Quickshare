#include "quickshare.h"
#include "gui.h"
#include "networking.h"
#include <windows.h>

// static Qs qs;

Qs::Qs()
{
    char user_name[32];
    DWORD size = sizeof(user_name);
    if (GetUserName(reinterpret_cast<TCHAR*>(user_name), &size) == 0)
        FATAL("Username buffer too small\n");
    snprintf(dir_path, 63, "C:\\Users\\%s\\AppData\\Local\\Quickshare", user_name);
    CreateDirectory(reinterpret_cast<TCHAR*>(dir_path), NULL);
}

int main(int argc, const char *argv[]) 
{
    Client_Sock client("192.168.1.31", 5000);
    if (!client.init_socket()) {
        LOGGER("Failed to init socket\n");
        exit(1);
    }
    Context ctx(&client);

    ctx.create_window(1000, 720, "Quickshare");
    ctx.init_imgui();
    ctx.main_loop();
    
    return 0;
}
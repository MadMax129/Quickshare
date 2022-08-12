#include "quickshare.h"
#include "gui.h"
#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    Context ctx{};

    ctx.create_window(WINDOW_WIDTH, WINDOW_HEIGHT, "Quickshare");
    ctx.init_imgui();
    ctx.main_loop();
    
    return 0;
}
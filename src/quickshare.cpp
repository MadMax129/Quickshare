#include "network.hpp"
#include "quickshare.hpp"
#include "file_manager.hpp"
#include <time.h>

static void init_qs()
{
#ifndef X86_64_CPU
    colored_print(CL_BLUE, "The platform detected is not x86-64."
                            "This could have undefined results.");
#endif
// #ifdef DEBUG_MODE
//     printf("QuickShare Debug Info:\n");
//     colored_printf(CL_GREEN, "OS:\t%s\n", SYSTEM_NAME);
//     colored_printf(CL_GREEN, "Arch:\t%s\n", ARCH);
// #endif
}

#ifdef SYSTEM_WIN_64
// int WINAPI WinMain(HINSTANCE hInstance,
//                    HINSTANCE hPrevInstance,
//                    LPSTR lpCmdLine,
//                    int nCmdShow)

int main(const int argc, const char* argv[])
#else
int main(const int argc, const char* argv[])
#endif
{
    File_Manager f_manager{};
    Network net(&f_manager);

    init_qs();
    net.network_loop();

    return 0;
}
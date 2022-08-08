#include "file_manager.hpp"
#include "network.hpp"
#include <string>

static void init_qs()
{
#ifndef X86_64_CPU
    colored_print(CL_BLUE, "The platform detected is not x86-64."
                            "This could have undefined results.");
#endif
}

QuickShare::QuickShare() 
{
    char temp[256];
#ifdef SYSTEM_WIN_64
    char user_name[32];
    DWORD size = sizeof(user_name);
    if (GetUserName((TCHAR*)user_name, &size) == 0)
        P_ERROR("User name length error\n");
    std::snprintf(temp, sizeof(char[256])-1, PATH_TO_DATA, user_name);
    CreateDirectory(reinterpret_cast<TCHAR*>(temp), NULL);
#endif
    dir_path = std::string(temp);
}

QuickShare qs{};

#ifdef SYSTEM_WIN_64
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
#else
int main(const int argc, const char* argv[])
#endif
{
    File_Sharing f_manager{};
    Network net(&f_manager);
    f_manager.add_network(&net);

    init_qs();
    net.network_loop();

    for (;;)
    {
        printf(
            "Temporary user menu:\n"
            "u - View users\n"
            "e - exit\n"
            "s - send file\n"
        );
        
        int c = getchar();
        
        switch (c) {
            case 'e': return 0;
            case 's': {
                UserId id;
                char fname[] = "../test_files/big.txt";
                scanf("%lld", &id);
                Users_List a = {std::make_pair(id, Msg::INVALID)};
                printf("Sending '%s' to '%lld'\n", fname, id);
                assert(f_manager.create_send(fname, a));
            }
        }
    }

    return 0;
}
/** @file quickshare.cpp
 * 
 * @brief Main entry point for quickshare
 *      
 * Copyright (c) 2022 Maks S.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */ 

#include "file_manager.hpp"
#include "network.hpp"
#include <string>

static void init_qs()
{
#ifndef X86_64_CPU
    colored_print(CL_BLUE, "The platform detected is not x86-64.\n"
                            "This could have undefined results.\n");
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
Allocation memory;

#ifdef SYSTEM_WIN_64
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
#else
int main(const int argc, const char* argv[])
#endif
{
    if (!memory.try_allocate())
        return 1;

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
                char fname[] = "../test_files/4kimage.jpg";
                (void)scanf("%lld", &id);
                Users_List a = {std::make_pair(id, Msg::INVALID)};
                printf("Sending '%s' to '%lld'\n", fname, id);
                assert(f_manager.create_send(fname, a));
            }
        }
    }

    memory.free();

    return 0;
}
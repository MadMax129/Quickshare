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

#include "util.hpp"
#include "locator.hpp"
#include "context.hpp"
#include "config.hpp"
#include "mem_pool.hpp"

#ifdef SYSTEM_WIN_64
#   include <windows.h>
#endif

struct Quickshare {
    bool init_all();
    void main();
    void end();
private:
#ifdef SYSTEM_WIN_64
    WSAData wsa_data;
#endif
};

static Quickshare qs;
Thread_Manager thread_manager;
Memory_Pool mem_pool;

bool Quickshare::init_all() 
{
#ifdef SYSTEM_WIN_64
    if (WSAStartup(MAKEWORD(2,2), &wsa_data) != 0) {
        P_ERROR("Failed to init 'WSAStartup'\n");
        return false;
    }
#endif
    return mem_pool.init();
}

void Quickshare::end()
{
#ifdef SYSTEM_WIN_64
    WSACleanup();
#endif
    mem_pool.cleanup();
}

void Quickshare::main()
{
    Context ctx;

    if (!ctx.create_window(WINDOW_WIDTH, WINDOW_HEIGHT, "Quickshare")) {
        P_ERROR("Failed to create window...");
        return;
    }

    ctx.init_imgui();
    ctx.main_loop();

    thread_manager.close_all();
    LOG("All threads closed...\n");
}

#ifdef SYSTEM_WIN_64
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
#else
int main(const int argc, const char* argv[])
#endif
{
#ifdef SYSTEM_WIN_64
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;
#endif
    if (qs.init_all()) {
        LOG("Starting Quickshare...\n");
        qs.main();
        qs.end();
    }
    else {
    #ifdef SYSTEM_WIN_64
        MessageBox(NULL, "Failed to initialize Quickshare...", "Quickshare", MB_ICONERROR | MB_OK);
    #endif
        P_ERROR("Failed to start Quickshare...\n");
    }

    return 0;
}
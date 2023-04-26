#include "util.hpp"
#ifdef SYSTEM_WIN_64
#   include <windows.h>
#elif defined(SYSTEM_UNX)
#   include <unistd.h>
#endif
#include <cstdio>
#include <mutex>
#include <cstring>
#include <cstdarg>

#include "config.hpp"

#ifndef RELEASE_MODE
static std::mutex log_mtx;

void _colored_print(void* color, const char* str, ...)
{
    log_mtx.lock();
    va_list ap;
	va_start(ap, str);

#if defined(SYSTEM_WIN_64)
    HANDLE hConsole;
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, (INT_PTR)color);
#elif defined(SYSTEM_UNX)
    std::fprintf(stdout, "%s", (char*)color);
#endif
	std::vfprintf(stdout, str, ap);
#if defined(SYSTEM_WIN_64)
    SetConsoleTextAttribute(hConsole, CL_RESET);
#elif defined(SYSTEM_UNX)
    std::fprintf(stdout, "%s", CL_RESET);
#endif

	va_end(ap);
    log_mtx.unlock();
}

#endif

void safe_strcpy(char* dest, const char* src, size_t size)
{
    std::memcpy(
        static_cast<void*>(dest),
        static_cast<const void*>(src),
        size
    );

    dest[size - 1] = '\0';
}

void get_computer_name(char* buffer)
{
#ifdef SYSTEM_WIN_64
    DWORD size = CLIENT_NAME_LEN;
    (void)GetComputerName((TCHAR*)buffer, &size);
#elif defined(SYSTEM_UNX)
    gethostname(buffer, CLIENT_NAME_LEN);
#endif
}
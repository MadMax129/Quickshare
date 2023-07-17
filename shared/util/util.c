#include "util.h"

#ifdef SYSTEM_WIN_64
#   include <windows.h>
#elif defined(SYSTEM_UNX)
#   include <unistd.h>
#endif
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdarg.h>

#ifndef RELEASE_MODE
// pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void _colored_print(const void* color, const char* str, ...)
{
    // pthread_mutex_lock(&log_mutex);
    va_list ap;
	va_start(ap, str);

#if defined(SYSTEM_WIN_64)
    HANDLE hConsole;
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, (INT_PTR)color);
#elif defined(SYSTEM_UNX)
    fprintf(stdout, "%s", (const char*)color);
#endif
	vfprintf(stdout, str, ap);
#if defined(SYSTEM_WIN_64)
    SetConsoleTextAttribute(hConsole, CL_RESET);
#elif defined(SYSTEM_UNX)
    fprintf(stdout, "%s", CL_RESET);
#endif
	va_end(ap);
    // pthread_mutex_unlock(&log_mutex);
}

#endif

void safe_strcpy(char* dest, const char* src, size_t size)
{
    memcpy(
        (void*)dest,
        (const void*)src,
        size
    );

    dest[size - 1] = '\0';
}

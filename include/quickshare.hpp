#pragma once

#include <stdio.h>
#include <stdarg.h>

#if defined(_WIN64)
   #define SYSTEM_WIN_64
   #include <windows.h>
#elif defined(__APPLE__) || defined(__MACH__)
   #define SYSTEM_MAC_64
   #define SYSTEM_UNX
#elif defined(__linux__)
   #define SYSTEM_LINUX_64
   #define SYSTEM_UNX
#else
   #define SYSTEM_ERROR
#endif

#ifdef __x86_64__
   #define X86_64_CPU
#endif

#include <cstdint>

typedef std::int8_t i8;
typedef std::int16_t i16;
typedef std::int32_t i32;
typedef std::int64_t i64;

typedef std::uint8_t u8;
typedef std::uint16_t u16;
typedef std::uint32_t u32;
typedef std::uint64_t u64;

#if defined(SYSTEM_UNX)
	#define CL_RESET  "\x1b[0m"
	#define CL_RED    "\x1b[31m"
	#define CL_BLUE   "\x1b[34m"
	#define CL_GREEN  "\x1b[32m"
	#define CL_YELLOW "\x1b[33m"
#elif defined(SYSTEM_WIN_64)
   #define CL_RESET  7
   #define CL_RED    4
   #define CL_BLUE   1	
   #define CL_GREEN  2
   #define CL_YELLOW 6
#endif

#define colored_printf(color, str, ...) \
    _colored_print((void*)color, str, __VA_ARGS__)
#define colored_print(color, str) \
    _colored_print((void*)color, str)

#define LOG(str) \
    colored_print(CL_YELLOW, str)

#define LOGF(str, ...) \
    colored_printf(CL_YELLOW, str, __VA_ARGS__)

static inline void _colored_print(void* color, const char* str, ...)
{
    va_list ap;
	va_start(ap, str);

#if defined(SYSTEM_WIN_64)
    HANDLE hConsole;
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, (INT_PTR)color);
#elif defined(SYSTEM_UNX)
    fprintf(stdout, "%s", color);
#endif
	vfprintf(stdout, str, ap);
#if defined(SYSTEM_WIN_64)
    SetConsoleTextAttribute(hConsole, CL_RESET);
#elif defined(SYSTEM_UNX)
    fprintf(stdout, "%s", CL_RESET);
#endif

	va_end(ap);
}
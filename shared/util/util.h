#ifndef QS_UTIL_HEADER
#define QS_UTIL_HEADER

#if defined(_WIN64)
#   define SYSTEM_WIN_64
#   define SYSTEM_NAME "Win64"
#elif defined(__APPLE__) || defined(__MACH__)
#   define SYSTEM_MAC_64
#   define SYSTEM_NAME "Mac64"
#   define SYSTEM_UNX
#elif defined(__linux__)
#   define SYSTEM_LINUX_64
#   define SYSTEM_NAME "Linux64"
#   define SYSTEM_UNX
#else
#   define SYSTEM_ERROR
#   error Unsupported OS 
#endif

#ifdef __x86_64__
#   define X86_64_CPU
#   define ARCH "X86-64"
#elif 
#   error Unsupported arch
#endif

#ifdef __cplusplus
#   include <cstdint>
#   include <cstdio>
#   include <ctime>
#else
#   include <stdint.h>
#   include <stdio.h>
#   include <time.h>
#endif
#include <assert.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef long long int i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef unsigned long long int u64;

typedef time_t UserId;

#ifdef SYSTEM_WIN_64
#   define PATH_TO_DATA "C:\\Users\\%s\\Downloads"
#elif defined(SYSTEM_UNX)
#   define PATH_TO_DATA "/home/%s/Downloads"
#endif

#if defined(SYSTEM_LINUX_64)
#   define CL_RESET  "\x1b[0m"
#   define CL_RED    "\x1b[31m"
#   define CL_BLUE   "\x1b[34m"
#   define CL_GREEN  "\x1b[32m"
#   define CL_YELLOW "\x1b[33m"
#elif defined(SYSTEM_MAC_64)
#   define CL_RESET  "\033[0m"
#   define CL_RED    "\033[31m"
#   define CL_BLUE   "\033[34m"
#   define CL_GREEN  "\033[32m"
#   define CL_YELLOW "\033[33m"
#elif defined(SYSTEM_WIN_64)
#   define CL_RESET  7
#   define CL_RED    4
#   define CL_BLUE   1	
#   define CL_GREEN  2
#   define CL_YELLOW 6
#endif

#define colored_printf(color, str, ...) \
    _colored_print((const void*)color, str, __VA_ARGS__)
#define colored_print(color, str) \
    _colored_print((const void*)color, str)

#define LOG(str) \
    colored_print(CL_YELLOW, "[ LOG ] " str);

#define LOGF(str, ...) \
    colored_printf(CL_YELLOW, "[ LOG ] " str, __VA_ARGS__);

#define P_ERROR(str) \
    colored_print(CL_RED, str)

#define P_ERRORF(str, ...) \
    colored_printf(CL_RED, str, __VA_ARGS__)

#ifdef RELEASE_MODE
#   undef LOG
#   undef LOGF
#   undef P_ERROR
#   undef P_ERRORF
#   define LOG(str)
#   define LOGF(str, ...)
#   define P_ERROR(str)
#   define P_ERRORF(str, ...)
#endif

#ifdef __cplusplus
extern "C" {
#endif 
void _colored_print(const void* color, const char* str, ...);
void safe_strcpy(char* dest, const char* src, size_t size);
#ifdef __cplusplus
}
#endif

#endif /* QS_UTIL_HEADER */

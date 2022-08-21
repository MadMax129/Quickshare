/** @file quickshare.hpp
 * 
 * @brief Main quickshare header
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

#pragma once

#include <cstdio>
#include <stdarg.h>
#include <string>

#if defined(_WIN64)
#   define SYSTEM_WIN_64
#   define SYSTEM_NAME "Win64"
#   include <WS2tcpip.h>
#   include <winsock2.h>
#   include <windows.h>
#   include <iphlpapi.h>
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

#include <cstdint>
#include <assert.h>

typedef std::int8_t i8;
typedef std::int16_t i16;
typedef std::int32_t i32;
typedef long long int i64;

typedef std::uint8_t u8;
typedef std::uint16_t u16;
typedef std::uint32_t u32;
typedef unsigned long long int u64;

#ifdef SYSTEM_WIN_64
    typedef SOCKET socket_t;
#elif defined(SYSTEM_UNX)
    typedef int socket_t;
#endif

#ifdef SYSTEM_WIN_64
#   define PATH_TO_DATA "C:\\Users\\%s\\Downloads"
#elif defined(SYSTEM_UNX)
#   define PATH_TO_DATA "/home/%s/Downloads"
#endif

#if defined(SYSTEM_UNX)
#   define CL_RESET  "\x1b[0m"
#   define CL_RED    "\x1b[31m"
#   define CL_BLUE   "\x1b[34m"
#   define CL_GREEN  "\x1b[32m"
#   define CL_YELLOW "\x1b[33m"
#elif defined(SYSTEM_WIN_64)
#   define CL_RESET  7
#   define CL_RED    4
#   define CL_BLUE   1	
#   define CL_GREEN  2
#   define CL_YELLOW 6
#endif

#define colored_printf(color, str, ...) \
    _colored_print((void*)color, str, __VA_ARGS__)
#define colored_print(color, str) \
    _colored_print((void*)color, str)

#define LOG(str) { \
    colored_print(CL_YELLOW, "[ LOG ] "); \
    std::printf(str); \
}

#define LOGF(str, ...) { \
    colored_print(CL_YELLOW, "[ LOG ] "); \
    std::printf(str, __VA_ARGS__); \
}

#define P_ERROR(str) \
    colored_print(CL_RED, str)

#define P_ERRORF(str, ...) \
    colored_printf(CL_RED, str, __VA_ARGS__)

void _colored_print(void* color, const char* str, ...);

struct QuickShare {
    QuickShare();
    std::string dir_path;
};

struct Allocation;

extern QuickShare qs;
extern Allocation memory;

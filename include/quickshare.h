#pragma once

#ifndef LOGGER
#define LOGGER(...) \
    do { printf("[ LOG ]\t" __VA_ARGS__); } while (0)
#endif

#ifndef FATAL
#define FATAL(...) \
    do { printf("[ FATAL ] (%s, %d)\t", __FILE__, __LINE__); printf(__VA_ARGS__); exit(1); } while (0)
#endif

#ifndef FATAL_MEM
#define FATAL_MEM() \
    do { printf("[ MEMORY FAILURE ] (%s, %d)\n", __FILE__, __LINE__); exit(1); } while (0)
#endif

#include <Lmcons.h>
struct Qs {
    Qs();
    void cache_icon();
    char dir_path[64];
};

#pragma once

#ifndef LOGGER
#define LOGGER(...) \
    do { printf("[ LOG ]\t" __VA_ARGS__); } while (0)
#endif

#ifndef FATAL
#define FATAL(...) \
    do { printf("[ FATAL ] (%s, %d)\t", __FILE__, __LINE__); printf(__VA_ARGS__); exit(1); } while (0)
#endif

#define USERNAME_MAX_LIMIT 16
#define DATA_MAX 512
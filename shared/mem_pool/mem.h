#ifndef QS_MEMORY
#define QS_MEMORY

#include <inttypes.h>
#include <stdbool.h>

typedef struct Block {
    struct Block* next;
    char *mem,
         *free,
         *end;
} Block;

typedef struct {
    Block root;
    size_t block_size;
    uint64_t mem_used;
} Memory_Pool;

bool mem_pool_init(size_t block_size);
void mem_pool_free();
char* alloc(size_t size);

#endif /* QS_MEMORY */
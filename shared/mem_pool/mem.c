#include <stdio.h>
#include <stdlib.h>

#include "mem.h"

static Memory_Pool mem_pool;

static bool init_block(Block* block)
{
    block->mem = (char*)malloc(mem_pool.block_size);

    if (!block->mem)
        return false;

    block->free = block->mem;
    block->next = NULL;
    block->end = block->mem + mem_pool.block_size;

    return true;
}

bool mem_pool_init(size_t block_size)
{
    mem_pool.mem_used = 0;
    mem_pool.block_size = block_size;

    if (!init_block(&mem_pool.root))
        return false;

    return true;
}

void mem_pool_free()
{
    free(mem_pool.root.mem);
}

char* alloc(size_t size)
{
    char *ptr = mem_pool.root.free;

    /* Pad for 8 byte alignment */
    while ((size % 8) != 0)
    	size++;

    if ((mem_pool.root.free + size) > mem_pool.root.end)
        return NULL;

    mem_pool.root.free += size;
    mem_pool.mem_used  += size;

    // printf(
    //     "Allocating: %lu %p Used: %lu\n", 
    //     size, 
    //     (void*)ptr, 
    //     mem_pool.mem_used
    // );

    return ptr;
}
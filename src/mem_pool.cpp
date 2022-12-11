#include "mem_pool.hpp"
#include "config.hpp"

bool Memory_Pool::init()
{
    if (!init_block(&main_mem))
        return false;

    used_mem = 0;

    return true;
}

void Memory_Pool::cleanup()
{
    std::free(main_mem.mem);
}

bool Memory_Pool::init_block(Block* block)
{
    block->mem = static_cast<char*>(std::malloc(BLOCK_SIZE));

    if (!block->mem)
        return false;

    block->free = block->mem;
    block->next = NULL;
    block->end = block->mem + BLOCK_SIZE;

    return true;
}

char* Memory_Pool::alloc(size_t size)
{
    char *ptr = main_mem.free;

    // Pad for 8 byte alignment
    while ((size % 8) != 0)
    	size++;

    if ((main_mem.free + size) >= main_mem.end) {
        P_ERROR("Cannot allocate large object\n");
        return NULL;
    }

    main_mem.free += size;
    used_mem      += size;

    LOGF("Allocating: %llu %p Used: %llu\n", size, (void*)ptr, used_mem);

    return ptr;
}
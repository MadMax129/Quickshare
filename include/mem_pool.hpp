#pragma once

#include "util.hpp"

class Memory_Pool {
public:
    bool init();
    void cleanup();

    char* alloc(size_t size);

private:
    struct Block {
        Block *next;
        char *mem, *free, *end;
    };

    bool init_block(Block* block); 

	u64 used_mem;
    Block main_mem;
};

extern Memory_Pool mem_pool;
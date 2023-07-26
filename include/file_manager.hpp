#pragma once

#include <stdio.h>
#include <array>
#include <optional>
#include <atomic>

#include "util.h"
#include "config.h"
#include "msg.h"

class File_Manager {
public:
    enum State {
        EMPTY,
        WORKING,
        COMPLETE
    };  
    
    File_Manager();

    bool read_file(const char* path);
    bool write_file(const char* filename);
    
    bool read_from_file(Packet* packet);
    bool write_to_file(Packet* packet);

private:
    i64 get_file_size(FILE* file_fd) const;
    std::atomic<State> state;
    std::atomic<u32> progress;
    FILE* file_fd;
    char* buf;
    u64 buf_len,
        file_size;   
};
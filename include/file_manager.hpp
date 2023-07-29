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
    enum Type {
        READ_FILE,
        WRITE_FILE
    };

    enum State {
        EMPTY,
        ERROR,
        WORKING,
        COMPLETE
    };  
    
    File_Manager();

    bool read_file(const char* path);
    bool write_file(const char* filename);

    inline State do_work(Packet* packet)
    {
        return type == READ_FILE ?
            read_from_file(packet) : 
            write_to_file(packet);
    }

private:
    State read_from_file(Packet* packet);
    State write_to_file(Packet* packet);
    i64 get_file_size() const;

    Type type;
    State state;
    std::atomic<u32> progress;
    FILE* file_fd;
    char* buf;
    u64 buf_len,
        file_size;   
};
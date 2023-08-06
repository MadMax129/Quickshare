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

    void init_read();
    void init_write();

    bool read_file(const char* path, i64& f_size);
    bool write_file(
        const char* filename, 
        const u64 f_size
    );
    void close();

    inline u32 get_progress() const 
    {
        return progress.load(std::memory_order_relaxed);
    }

    State read_work(Packet* packet);
    State write_work(const Transfer_Data* t_data);

private:
    bool read_from_file(Packet* packet);
    i64 get_file_size() const;

    Type type;
    State state;
    std::atomic<u32> progress;
    FILE* file_fd;
    char* buf;
    u64 buf_len,
        buf_cnt,
        file_size;
};
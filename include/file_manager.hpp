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

    struct Session {
        Session() :
            state(EMPTY),
            progress(0),
            file_fd(NULL),
            buf(NULL),
            buf_len(0),
            file_size(0) 
        {}

        std::atomic<State> state;
        std::atomic<u32> progress;
        FILE* file_fd;
        char* buf;
        u64 buf_len,
            file_size;    
    };
    
    using Transfer_Array = std::array<Session, SIM_TRANSFERS_MAX>;

    File_Manager();

    Session* read_file(const char* path);
    Session* write_file(const char* filename);
    
    bool read_from_file(Packet* packet);
    bool write_to_file(Packet* packet);

private:
    i64 get_file_size(FILE* file_fd) const;
    Session* get_session(Transfer_Array& t_array);
    Session* get_work();

    Transfer_Array write_transfers;
    char cache_line_pad[64];
    Transfer_Array read_transfers;
};
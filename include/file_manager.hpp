#pragma once

#include <stdio.h>
#include <array>
#include <optional>

#include "util.h"
#include "config.h"
#include "msg.h"

class File_Manager {
public:
    enum State {
        EMPTY,
        READING,
        WRITING,
        COMPLETE
    };  

    struct Session {
        State state;
        FILE* file_fd;
        char* buf;
        u64 buf_len,
            file_size,
            left_to_read;
    };
    
    File_Manager();

    bool read_file(const char* path);
    bool write_file(const char* filename);
    
    bool make_data_packet(Packet* packet);
    bool read_data_packet(Packet* packet);

private:
    i64 get_file_size(FILE* file_fd) const;

    Session session;
};
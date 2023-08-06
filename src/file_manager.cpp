#include <algorithm>
#include <assert.h>
#include <string>
#include <cstring>
#include <new>

#include "file_manager.hpp"
#include "mem.h"

File_Manager::File_Manager() :
    state(EMPTY),
    progress(0),
    file_fd(NULL),
    buf(NULL),
    buf_len(0)
{}

void File_Manager::init_read()
{
    type = READ_FILE;
    buf  = alloc(FILE_BUFFER_SIZE);
    if (!buf) {
        P_ERROR("Failed to alloc buffer\n");
        exit(EXIT_FAILURE);
    }
}

void File_Manager::init_write()
{
    type = WRITE_FILE;
}

i64 File_Manager::get_file_size() const
{
    if (std::fseek(file_fd, 0, SEEK_END) != 0)
        return -1;

    const i64 f_size = std::ftell(file_fd);

    std::rewind(file_fd);

    return f_size;
}

bool File_Manager::read_file(const char* path, i64& f_size)
{
    assert(type == READ_FILE);

    file_fd = std::fopen(path, "rb");

    if (
        !file_fd ||
        (file_size = get_file_size()) <= 0
    ) {
        return false;
    }

    f_size   = file_size;
    buf_len  = 0;
    buf_cnt  = 0;
    state    = WORKING;
    progress.store(0, std::memory_order_relaxed);
    
    return true;
}

bool File_Manager::write_file(const char* filename, const u64 f_size)
{
    assert(type == WRITE_FILE);

    const char* env_name;

#ifdef SYSTEM_WIN_64
    env_name = std::getenv("USERNAME");
#elif defined(SYSTEM_UNX)
    env_name = std::getenv("USER");
#endif

    if (!env_name)
        return false;

    std::string download_path;
#ifdef SYSTEM_WIN_64
    download_path = std::string("C:\\Users\\")   + 
                    env_name                     + 
                    std::string("\\Downloads\\") + 
                    filename;
#elif defined(SYSTEM_UNX)
    download_path = std::string("/home/")      + 
                    env_name                   + 
                    std::string("/Downloads/") + 
                    filename;
#endif

    file_fd = std::fopen(download_path.c_str(), "wb");

    if (!file_fd)
        return false;

    state     = WORKING;
    file_size = f_size;
    buf_len   = 0;

    progress.store(0, std::memory_order_relaxed);

    return true;
}

void File_Manager::close()
{
    state = EMPTY;
    std::fclose(file_fd);
}

bool File_Manager::read_from_file(Packet* packet)
{
    assert(file_fd);
    if (buf_len == 0) {
        buf_len = std::fread(
            buf,
            1,
            FILE_BUFFER_SIZE,
            file_fd
        );

        if (buf_len == 0 && std::ferror(file_fd)) {
            return false;
        }
        else if (buf_len == 0 && std::feof(file_fd)) {
            state = COMPLETE;
            return true;
        }
        buf_cnt = 0;
    }

    PACKET_HDR(
        P_TRANSFER_DATA,
        sizeof(packet->d.transfer_data),
        packet
    );

    u64 send_size = std::min(
        buf_len, 
        (u64)sizeof(packet->d.transfer_data.bytes)
    );

    packet->d.transfer_data.b_size = send_size;
    buf_len -= send_size;

    (void)std::memcpy(
        packet->d.transfer_data.bytes,
        buf + buf_cnt,
        send_size
    );
    buf_cnt += send_size;

    progress.store(
        (std::ftell(file_fd) * 100) / file_size,
        std::memory_order_relaxed 
    );

    return true;
}

File_Manager::State File_Manager::read_work(Packet* packet)
{
    switch (state)
    {
        case WORKING:
            if (!read_from_file(packet))
                state = ERROR;
            break;

        case COMPLETE:
        case ERROR:
        case EMPTY:
            break;
    }

    return state;
}

File_Manager::State File_Manager::write_work(const Transfer_Data* t_data)
{
    switch (state)
    {
        case WORKING: {
            const size_t bytes = fwrite(
                t_data->bytes,
                1,
                t_data->b_size,
                file_fd
            );

            assert(bytes == t_data->b_size);
            buf_len += bytes;

            progress.store(
                (buf_len * 100) / file_size,
                std::memory_order_relaxed
            );
            break;
        }

        case COMPLETE:
        case ERROR:
        case EMPTY:
            break;
    }

    return state;
}
#include <algorithm>
#include <assert.h>
#include <string>
#include <cstring>

#include "file_manager.hpp"
#include "mem.h"

File_Manager::File_Manager() : 
    session({}) 
{
    session.buf = alloc(1024 * 12);

    if (!session.buf) {
        P_ERROR("File Mangaer buffer alloc failed\n");
        exit(EXIT_FAILURE);
    }
}

i64 File_Manager::get_file_size(FILE* file_fd) const
{
    assert(file_fd);

    if (fseek(file_fd, 0, SEEK_END) != 0)
        return -1;

    return ftell(file_fd);
}

bool File_Manager::read_file(const char* path)
{
    session.file_fd = std::fopen(path, "rb");
    i64 file_size;

    if (!session.file_fd ||
        (file_size = get_file_size(session.file_fd)) == -1
    ) {
        return false;
    }

    session.file_size = session.left_to_read = file_size;
    session.buf_len = 0;
    session.state = State::READING;
    
    return true;
}

bool File_Manager::write_file(const char* filename)
{
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

    LOGF("PATH %s\n", download_path.c_str());
    assert(false);

    session.file_fd = std::fopen(download_path.c_str(), "wb");

    if (!session.file_fd)
        return false;

    /* Pass in file size !! */
    // session.file_size = session.left_to_read = file_size;
    // session.buf_len = 0;
    session.state = State::WRITING;

    return true;
}

bool File_Manager::make_data_packet(Packet* packet)
{
    if (session.state == READING) {

        if (session.buf_len == 0) {
            // Read buf from file
            ssize_t n = std::fread(
                session.buf, 
                1, 
                1024 * 12, 
                session.file_fd
            );

            session.buf_len = n;
        }

        assert(session.buf_len <= sizeof(packet->d.transfer_data.bytes));

        PACKET_HDR(
            P_TRANSFER_DATA,
            sizeof(packet->d.transfer_data),
            packet
        );

        packet->d.transfer_data.b_size = session.buf_len;
        (void)std::memcpy(
            packet->d.transfer_data.bytes,
            session.buf,
            session.buf_len
        );

        session.state = COMPLETE;
    } else {
        LOG("COMPLETE\n");
        return false;
    }

    return true;
}

bool File_Manager::read_data_packet(Packet* packet)
{
    (void)packet;
    return true;
}
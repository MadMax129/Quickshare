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
    buf_len(0),
    file_size(0)
{}

i64 File_Manager::get_file_size() const
{
    if (fseek(file_fd, 0, SEEK_END) != 0)
        return -1;

    return ftell(file_fd);
}

bool File_Manager::read_file(const char* path)
{
    type = READ_FILE;

    file_fd = std::fopen(path, "rb");
    i64 file_size;

    if (!file_fd ||
        (file_size = get_file_size()) == -1
    ) {
        return false;
    }

    file_size = file_size;
    buf_len = 0;
    
    state = WORKING;
    progress.store(0, std::memory_order_relaxed);
    
    return true;
}

// File_Manager::Session* File_Manager::write_file(const char* filename)
// {
//     const char* env_name;

// #ifdef SYSTEM_WIN_64
//     env_name = std::getenv("USERNAME");
// #elif defined(SYSTEM_UNX)
//     env_name = std::getenv("USER");
// #endif
//     Session* session = get_session(write_transfers);
//     if (!env_name || !session)
//         return NULL;

//     std::string download_path;
// #ifdef SYSTEM_WIN_64
//     download_path = std::string("C:\\Users\\")   + 
//                     env_name                     + 
//                     std::string("\\Downloads\\") + 
//                     filename;
// #elif defined(SYSTEM_UNX)
//     download_path = std::string("/home/")      + 
//                     env_name                   + 
//                     std::string("/Downloads/") + 
//                     filename;
// #endif

//     LOGF("PATH %s\n", download_path.c_str());

//     progress.store(0, std::memory_order_relaxed);
//     session->state.store(WORKING, std::memory_order_release);

//     // session.file_fd = std::fopen(download_path.c_str(), "wb");

//     // if (!session.file_fd)
//     //     return false;

//     // /* Pass in file size !! */
//     // // session.file_size = session.left_to_read = file_size;
//     // // session.buf_len = 0;
//     // session.state = State::WRITING;

//     return session;
// }

// File_Manager::Session* File_Manager::get_work()
// {
//     for (u32 i = 0; i < SIM_TRANSFERS_MAX; i++) {
//         const State s1 = read_transfers[i].state.load(
//             std::memory_order_acquire
//         );

//         if (s1 == WORKING || s1 == COMPLETE)
//             return &read_transfers[i];
//     }

//     return NULL;
// }

File_Manager::State File_Manager::read_from_file(Packet* packet)
{
    (void)packet;
    switch (state)
    {
        case ERROR:
        case WORKING: {
            // if (buf_len == 0) {
            //     buf_len = std::fread(
            //         buf, 
            //         1, 
            //         1024 * 12, 
            //         file_fd
            //     );

            //     assert(buf_len);
            // }

            // printf("==>%s\n", buf);

            // assert(buf_len <= sizeof(packet->d.transfer_data.bytes));
            // assert(buf_len == file_size);

            // PACKET_HDR(
            //     P_TRANSFER_DATA,
            //     sizeof(packet->d.transfer_data),
            //     packet
            // );

            // packet->d.transfer_data.b_size = buf_len;
            // (void)std::memcpy(
            //     packet->d.transfer_data.bytes,
            //     buf,
            //     buf_len
            // );

            // state.store(COMPLETE, std::memory_order_release);

            break;
        }
        
        case COMPLETE:
            LOG("COMPLETE\n");
            break;

        case EMPTY:
            break;
    }

    return state;
}

// bool File_Manager::write_to_file(Packet* packet)
// {
//     (void)packet;
//     return true;
// }
//
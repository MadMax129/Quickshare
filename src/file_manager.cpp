/** @file file_manager.cpp
 * 
 * @brief File sharing module
 *      
 * Copyright (c) 2022 Maks S.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */ 

#include "file_manager.hpp"
#include <cstring>
#include <string>
#include <algorithm>
#include <iterator>

File_Sharing::File_Sharing() : r_msg_queue(MAX_RECV_QUEUE_SIZE)
{
    temp_msg = memory.get_msg(1);
    s_data.buffer = memory.get_buffer(0);
    r_data.buffer = memory.get_buffer(1);

    s_data.thread = std::thread(&File_Sharing::send_loop, this);
    s_data.thread.detach();

    r_data.thread = std::thread(&File_Sharing::recv_loop, this);
    r_data.thread.detach();
}

File_Sharing::~File_Sharing() { }

void File_Sharing::cleanup()
{
    assert(s_data.state.load(std::memory_order_relaxed) != ACTIVE);
    s_data.state.store(INACTIVE, std::memory_order_relaxed);
    r_data.state.store(INACTIVE, std::memory_order_relaxed);
}

bool File_Sharing::create_send(const char* fname, Users_List users)
{
    // Check if not already sending
    if (s_data.state.load(std::memory_order_relaxed) != Transfer_State::INACTIVE)
        return false;

    std::unique_lock<std::mutex> lk(s_data.lock);

    s_data.users = users;
    s_data.file = std::fopen(fname, "rb");
    if (!s_data.file)
        return false;
    std::fseek(s_data.file, 0, SEEK_END);
    s_data.hdr.file_size = std::ftell(s_data.file);
    std::rewind(s_data.file);
    
    std::strncpy((char*)s_data.hdr.file_name, fname, MAX_FILE_NAME - 1);
    s_data.hdr.packets = packets_in_file(s_data.hdr.file_size);

    s_data.state.store(Transfer_State::REQUESTED, std::memory_order_relaxed);
    s_data.progress.store(0, std::memory_order_relaxed);


    s_data.cond.notify_one();

    return true;
}

u64 File_Sharing::packets_in_file(u64 file_size)
{
    u64 packets = 0;
    u64 total_bytes = 0;
    const u64 n_bufs = (u64)std::ceil((double)file_size / MEGABYTE);

    for (u64 b = 0; b < n_bufs; b++) {
        u64 b_read = 0;
        if ((total_bytes + MEGABYTE) > file_size) {
            b_read = file_size - total_bytes;
            total_bytes += file_size - total_bytes;
        }
        else {
            total_bytes += MEGABYTE;
            b_read = MEGABYTE;
        }

        packets += (u64)std::ceil((double)b_read / sizeof(Msg::packet.bytes));
    }

    assert(total_bytes == file_size);

    return packets;
}

void File_Sharing::send_loop()
{   
    for (;;) {
        std::unique_lock<std::mutex> lk(s_data.lock);
        s_data.cond.wait(lk);

        MSG_TYPE(temp_msg, Msg::REQUEST);
        MSG_SENDER(temp_msg, network->my_id);
        std::memcpy(&temp_msg->request, &s_data.hdr, sizeof(Request));

        for (auto& user : s_data.users) {
            LOGF("Sending request message to '%lld'\n", user.first);

            temp_msg->hdr.recipient_id = user.first;
            if (!network->send_to_id(temp_msg, user.first)) {
                LOGF("Rejecting '%lld' due to disconnection...\n", user.first);
                user.second = Msg::REJECTED;
            }
        }

        LOG("Waiting on all users response...\n");

        s_data.cond.wait_for(lk, std::chrono::seconds(30), [&] {
            for (const auto &user : s_data.users) {
                if (user.second == Msg::INVALID)
                    return false;
            }
            return true;
        });

        LOG("Got responses\n");
        for (auto &u : s_data.users)
            LOGF("\t'%lld' -> %s\n", 
                u.first, 
                u.second == Msg::ACCEPTED 
                ? "Accepted" : "Rejected");

        s_data.state.store(ACTIVE, std::memory_order_relaxed);
        
       send_packets();
    }
}

void File_Sharing::got_response(const Msg* msg)
{
    s_data.lock.lock();

    auto e = std::find_if(
        std::begin(s_data.users),
        std::end(s_data.users),
        [&] (const auto& p) {
            return p.first == msg->hdr.sender_id;
        }
    );

    assert(e != std::end(s_data.users));

    e->second = msg->hdr.type;
    
    s_data.lock.unlock();
    s_data.cond.notify_one();
}

void File_Sharing::send_packets()
{
    const u64 n_reads = (u64)std::ceil(((double)s_data.hdr.file_size / MEGABYTE));
    
    for (u32 mb = 0; mb < n_reads; mb++) {
        const u64 f_read = std::fread(
            s_data.buffer,
            sizeof(u8),
            MEGABYTE,
            s_data.file
        );

        const u64 n_packets = (u64)std::ceil((double)f_read / sizeof(Msg::packet.bytes));

        u64 p_progress = 0;
        for (u64 p = 0; p < n_packets; p++) {
            MSG_TYPE(temp_msg, Msg::PACKET);
            MSG_SENDER(temp_msg, network->my_id);

            // Determine packet size
            if ((p_progress + sizeof(Msg::packet.bytes)) > f_read) {
                temp_msg->packet.packet_size = f_read - p_progress;
            }
            else {
                p_progress += sizeof(Msg::packet.bytes);
                temp_msg->packet.packet_size = sizeof(Msg::packet.bytes);
            }

            // printf("Sending packet %llu size %u\n", p, temp_msg->packet.packet_size);

            std::memcpy(
                temp_msg->packet.bytes, 
                s_data.buffer + p * sizeof(Msg::packet.bytes),
                temp_msg->packet.packet_size
            );

            for (auto &user : s_data.users) {
                temp_msg->hdr.recipient_id = user.first;

                if (user.second == Msg::ACCEPTED) {
                    if (!network->send_to_id(temp_msg, user.first)) {
                        LOGF("Failed to send a packet, user '%lld' is now rejected\n", user.first);
                        user.second = Msg::REJECTED;
                    }
                }
            }
            s_data.progress.fetch_add(1, std::memory_order_relaxed);
        }
    }

    /* old fread constantly
    for (u32 packet = 0; packet < s_data.hdr.packets; packet++) {
        std::memset(temp_msg, 0, sizeof(Msg));
        temp_msg->hdr.type = Msg::PACKET;
        temp_msg->hdr.sender_id = network->my_id;

        // Eventually switch to reading large MEGA byte buffer 
        size_t read = std::fread(
            temp_msg->packet.bytes, 
            sizeof(u8), 
            sizeof(Msg::packet.bytes),
            s_data.file);
    
        temp_msg->packet.packet_size = read;

        for (auto &user : s_data.users) {
            temp_msg->hdr.recipient_id = user.first;

            if (user.second == Msg::ACCEPTED) {
                if (!network->send_to_id(temp_msg, user.first)) {
                    LOGF("Failed to send a packet, user '%lld' is now rejected\n", user.first);
                    user.second = Msg::REJECTED;
                }
            }
        }
        s_data.progress.fetch_add(1, std::memory_order_relaxed);
    }
    */

    std::fclose(s_data.file);
    s_data.state.store(FINISHED, std::memory_order_relaxed);
}

bool File_Sharing::create_recv(const Request *r_hdr, UserId from)
{
    if (r_data.state.load(std::memory_order_relaxed) != Transfer_State::INACTIVE)
        return false;

    std::unique_lock<std::mutex> lk(r_data.lock);
    r_data.users.push_back(std::make_pair(from, Msg::INVALID));
    std::memcpy(&r_data.hdr, r_hdr, sizeof(Request));
    r_data.state.store(Transfer_State::REQUESTED, std::memory_order_relaxed);
    r_data.progress.store(0, std::memory_order_relaxed);

    return true;
}

void File_Sharing::push_msg(const Msg* msg)
{
    if (!r_msg_queue.try_push(*msg)) {
        P_ERRORF("Push failed at %u\n", r_data.progress.load());
        exit(1);
    }
}

void File_Sharing::accept_request()
{
    if (r_data.state.load(std::memory_order_relaxed) != Transfer_State::REQUESTED)
        return;

    std::unique_lock<std::mutex> lk(r_data.lock);

    std::string file = r_data.hdr.file_name;
    file = file.substr(file.find_last_of("/\\") + 1);
    std::string path = qs.dir_path + "\\" + file;
    
    printf("==>%s\n", path.c_str());

    r_data.file = std::fopen(path.c_str(), "wb");
    r_data.state.store(ACTIVE, std::memory_order_relaxed);

    r_data.cond.notify_one();
}

void File_Sharing::recv_loop()
{
    for (;;) {
        bool failed = false;
        std::unique_lock<std::mutex> lk(r_data.lock);
        r_data.cond.wait(lk);

        /* Consider the examples:
            3 Clients:
            C1 C2 and C3 (aka server)
            C1 recv from C2
            C2 disconnectes however C1 only knows about server
            C2 must check updated client list and decided to call fail_recv()
        */

        for (u32 p = 0; p < r_data.hdr.packets; p++) {
            const Msg* msg = nullptr;
            while (((msg = r_msg_queue.front()) == nullptr) &&
                r_data.state.load(std::memory_order_relaxed) == ACTIVE) 
                ;
            
            msg = r_msg_queue.front();

            if (r_data.state.load(std::memory_order_relaxed) == FAILED) {
                LOG("Recv failed exiting loop...\n");
                failed = true;
                break;
            }

            std::fwrite(
                (char*)msg->packet.bytes, 
                sizeof(u8), 
                msg->packet.packet_size, 
                r_data.file);

            r_data.progress.fetch_add(1, std::memory_order_relaxed);
            // LOGF("Recieved %u / %llu packets\n", 
            //     r_data.progress.load(std::memory_order_relaxed), 
            //     r_data.hdr.packets);

            r_msg_queue.pop();
        }

        if (!failed) {
            r_data.state.store(FINISHED, std::memory_order_relaxed);
            colored_print(CL_GREEN, "Success recv file\n");
        }
        else {
            P_ERROR("Failed to complete recv file...\n");
        }
        std::fclose(r_data.file);
    }
}

void File_Sharing::fail_recv()
{
    r_data.state.store(FAILED, std::memory_order_relaxed);
}

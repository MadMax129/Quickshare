#pragma once

#include "network.hpp"
#include "quickshare.hpp"
#include <mutex>
#include <cstdio>
#include <cmath>

enum Transfer_State {
    INACTIVE,
    REQUESTED,
    SENDING,
    RECEVING
};

struct File_Manager {
    File_Manager() = default;
    ~File_Manager() = default;

    inline void set_state(Transfer_State state)
    {
        this->state.store(state);
    }

    inline Transfer_State get_state()
    {
        return state.load();
    }

    inline void set_recv_hdr(const Request *hdr)
    {
        // Only done if state is INACTIVE
        // Meaning GUI cannot be reading this before then
        this->hdr = *hdr;
    }

    bool set_send_hdr(const char* fname, Request* req)
    {
        std::FILE* temp = std::fopen(fname, "r");

        if (!temp)
            return false;

        std::fseek(temp, 0, SEEK_END);
        i64 size = std::ftell(temp);
        std::rewind(temp);
        std::fclose(temp);

        std::strncpy(hdr.file_name, fname, MAX_FILE_NAME);
        hdr.file_size = size;
        hdr.packets = std::ceil(((double)size / PACKET_MAX_SIZE));
        *req = hdr;

        return true;
    }

    inline void start_transfer(const char* name=NULL)
    {
        // Add later downloade directory for quickshare
        Transfer_State s = state.load();
        const char* mode;

        assert(s == Transfer_State::SENDING || s == Transfer_State::RECEVING);

        if (s == Transfer_State::SENDING)
            mode = "rb";
        else
            mode = "wb";

        if (!name)
            out_file = fopen(hdr.file_name, mode);
        else
            out_file = fopen(name, mode);
        recv_packets = 0;
    }

    inline void end_transfer()
    {
        std::fclose(out_file);
    }

    void write(const Msg& msg)
    {
        recv_packets.fetch_add(1, std::memory_order_relaxed);
        std::fwrite((char*)&msg.packet.bytes, sizeof(char), msg.packet.packet_size, out_file);
        LOGF("Recieved %u / %u packets\n", recv_packets.load(std::memory_order_relaxed), hdr.packets);
    }

    bool read(Msg& msg)
    {
        bool more = true;
        u32 i;
        // read from file and package into packet
        for (i = 0; i < sizeof(msg.packet.bytes); i++) {
            i32 c = std::fgetc(out_file);
            if (c != EOF)
                msg.packet.bytes[i] = c;
            else {
                more = false;
                break;
            }
        }
        msg.packet.packet_size = i;
        recv_packets.fetch_add(1, std::memory_order_relaxed);

        return more;
    }

private:
    std::atomic<Transfer_State> state{INACTIVE};
    std::atomic<u32> recv_packets{0};
    std::FILE *out_file;
    Request hdr;
};
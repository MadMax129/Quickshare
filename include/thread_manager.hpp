#pragma once

#include <array>
#include <string>
#include <thread>
#include "state.hpp"
#include "util.h"
#include "config.h"

#define MAX_THREAD_NUMBER 8

using Status = State_Manager<bool>;

class Thread_Manager {
public:
    Thread_Manager() : th_num(0) {} 

    struct Slot {
        std::thread th;
        Status active;
    };

    template<class Func, class ...Args>
    bool new_thread(Func&& f, Args&&... args)
    {
        if (th_num == MAX_THREAD_NUMBER)
            return false;

        auto &slot = thread_map.at(th_num);
        slot.active.set(true);
        slot.th = std::thread(f, args..., std::ref(slot.active));
        ++th_num;
        return true;
    }

    void close_all()
    {
        for (u32 i = 0; i < th_num; i++) {
            auto& slot = thread_map.at(i);
            slot.active.set(false);
            if (slot.th.joinable())
                slot.th.join();
        }
        th_num = 0;
    }
    
private:
    std::array<Slot, MAX_THREAD_NUMBER> thread_map;
    u32 th_num;
};

extern Thread_Manager thread_manager;
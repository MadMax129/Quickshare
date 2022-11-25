#pragma once

#include <unordered_map>
#include <string>
#include <thread>
#include "state.hpp"

using Status = State_Manager<bool>;

class Thread_Manager {
public:
    struct Slot {
        Slot() = default;
        std::thread th;
        Status active;
    };

    template<class Func, class ...Args>
    Status& new_thread(std::string name, Func&& f, Args&&... args)
    {
        thread_map.insert(std::unordered_map<std::string, Slot>::value_type({name, Slot()}));

        Slot& slot = thread_map.at(name);

        slot.active.set(true);
        slot.th = std::thread(f, args..., std::ref(slot.active));

        return slot.active;
    }

    void close_all()
    {
        for (auto& i : thread_map) {
            i.second.active.set(false);
            if (i.second.th.joinable())
                i.second.th.join();
        }
    }
    

private:
    std::unordered_map<std::string, Slot> thread_map;
};
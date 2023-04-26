#pragma once

#include <atomic>

template <typename T>
struct State_Manager {
    State_Manager() = default;
    State_Manager(const State_Manager<T>& rhs) : state(rhs.state.load(std::memory_order_relaxed)) {}

    inline T get(std::memory_order order = std::memory_order_relaxed)
    {
        return state.load(order);
    }

    inline void set(T value, std::memory_order order = std::memory_order_relaxed)
    {
        state.store(value, order);
    }

private:
    std::atomic<T> state;
};
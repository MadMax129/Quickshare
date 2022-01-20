#pragma once

#include <mutex>
#include "networking.h"
#include "quickshare.h"

template <class T>
struct Safe_Queue {
public:
    Safe_Queue(const int queue_len);
    ~Safe_Queue();

    void pop(T const* buf);
    template <typename Peek_T>
    Peek_T peek();
    void push(T const* item);
    const int get_size();
    
private:
    const int queue_len;
    int front, back, size;
    T* queue;
    std::mutex mutex;
};

template <class T>
Safe_Queue<T>::Safe_Queue(const int queue_len) : queue_len(queue_len)
{
    front = 0;
    back = -1;
    size = 0;
    queue = (T*)malloc(sizeof(T) * queue_len);
    if (!queue)
        FATAL_MEM();
    memset(queue, 0, sizeof(T[queue_len]));
}

template <class T>
Safe_Queue<T>::~Safe_Queue() 
{
    free(queue);
}

template <class T>
void Safe_Queue<T>::pop(T const* buf)
{
    mutex.lock();
    assert(size >= 0); // eventually add bool

    memcpy(buf, &queue[front++], sizeof(T));

    if (front == queue_len)
        front = 0;

    size--;
    mutex.unlock();
}

template <class T, typename Peek_T>
Peek_T Safe_Queue<T>::peek()
{
    mutex.lock();
    Peek_T copy = queue[front].m_type;
    mutex.unlock();
    return copy;
}

template <class T>
void Safe_Queue<T>::push(T const* item)
{
    mutex.lock();
    assert(size != queue_len); // add bool

    if (back == queue_len-1)
        back = -1;

    back++;
    memcpy(&queue[back], item, sizeof(T));
    size++;
    mutex.unlock();
}

template <class T>
const int Safe_Queue<T>::get_size() 
{
    mutex.lock();
    const int s = size;
    mutex.unlock();
    return s;
}

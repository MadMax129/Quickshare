#include <iostream>
#include "../lib/SPSCQueue.h"
#include "../lib/LockFreeQueueCpp11.h"
#include "../include/msg.hpp"
#include "../include/queue.hpp"

// rigtorp::SPSCQueue<Msg> q(10);
// LockFreeQueueCpp11<Msg> q(10);
Queue q(10);

int main() {
    static Msg m = {}, m2 = {};
    m.hdr.type = Msg::ACCEPTED;
    assert(q.push(&m));

    m2.hdr.type = Msg::REJECTED;
    q.push(&m2);

    assert(q.front()->hdr.type == Msg::ACCEPTED);
    q.pop();
    assert(q.front()->hdr.type == Msg::REJECTED);
    q.pop();
    // assert(q.pop());

}
#include "thread_manager.hpp"
#include "util.hpp"
#include <unordered_map>
#include <string>
#include "state.hpp"
#include "unistd.h"

void func(Status& state)
{
    while (state.get() == true) ;
}

int main()
{
    Thread_Manager th;
    th.new_thread(func);

    th.close_all();

    colored_print(CL_GREEN, "[ OK ] ");
    printf("thread_test.cpp 'Thread Manager Working'\n");
}   
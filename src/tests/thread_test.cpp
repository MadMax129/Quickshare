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


    Status& s = th.new_thread("Test", func);

    sleep(1);
    s.set(false);
    sleep(1);

    th.close_all();

    colored_print(CL_GREEN, "[ Ok ] ");
    printf("Thread Test\n");
}   
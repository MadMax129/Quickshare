#include "connection.hpp"
#include "util.hpp"
#include <thread>
#include <mutex>

#ifdef SYSTEM_WIN_64
#   include <windows.h>
#endif

#define EXPECTED_VALUE 0xfeed

static std::mutex m;
static int got;

void client()
{
    m.lock();
    Connection<i32> conn;
    int s = EXPECTED_VALUE;

    if (conn.create_socket("127.0.0.1", 8080)) {
        if (conn.connect()) {
            if (conn.send(conn.me(), &s)) {
                conn.close();
                return;
            }         
        }
    }

    colored_print(CL_RED, "[ FAILED ] ");
    std::puts("conn_test.cpp 'client init failed'\n");
    exit(-1);
}

void server()
{
    Connection<i32> conn;
    
    if (!conn.create_socket("127.0.0.1", 8080)) {
        colored_print(CL_RED, "[ FAILED ] ");
        std::puts("conn_test.cpp 'Error creating socket'\n");
        exit(-1);
    }
    
    if (conn.bind_and_listen()) {
        m.unlock();
        const Sock_Info i = conn.accept();
        if (i.has_value()) {
            if (conn.recv(i.value().first, &got)) {
                CLOSE_SOCKET(i.value().first);
                conn.close();
                return;
            }
        }
    }

    conn.close();
    colored_print(CL_RED, "[ FAILED ] ");
    std::puts("conn_test.cpp 'server init failed'\n");
    exit(-1);
}

int main()
{
#ifdef SYSTEM_WIN_64
    WSAData wsa_data;
    if (WSAStartup(MAKEWORD(2,2), &wsa_data) != 0) {
        P_ERROR("Failed to init 'WSAStartup'\n");
        return false;
    }
#endif
    m.lock();
    std::thread s(server);
    std::thread c(client);

    s.join();
    c.join();

    if (got == EXPECTED_VALUE) {
        colored_print(CL_GREEN, "[ OK ] ");
        std::printf("conn_test.cpp 'Result = 0x%x'\n", EXPECTED_VALUE);
    }
    else {
        colored_print(CL_GREEN, "[ FAILED ] ");
        std::puts("conn_test.cpp 'got wrong result'\n");
    }
#ifdef SYSTEM_WIN_64
    WSACleanup();
#endif
}
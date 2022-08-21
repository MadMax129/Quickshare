#include "../../include/quickshare.hpp"

#ifdef SYSTEM_WIN_64
#   include <winsock2.h>
#   include <WS2tcpip.h>
#elif defined(SYSTEM_UNX)
#   include <sys/socket.h>
#   include <unistd.h>
#   include <arpa/inet.h>
#endif

/* Test most efficient packet send size tcp 
   on both windows and unx
*/

#include <cstdio>
#include <thread>
#include <ctime>
#include <atomic>
#include <cmath>

struct Info {
#ifdef SYSTEM_WIN_64
    SOCKET sock;
#elif defined(SYSTEM_UNX)
    int sock;
#endif
    struct sockaddr_in addr;
    int len;
};

struct {
#ifdef SYSTEM_WIN_64
    struct WSAData wsa_data;
#endif
    Info client[2];
} static ctx;

static std::atomic<bool> s_state{false};
static std::atomic<bool> c_state{false};

#define PACKET_SIZE (1278)
#define MEGABYTE (1024*1024)
#define MTU_SIZE (1472) // Measured on my pc

void server()
{
    const char* buffer;
    const auto& me = ctx.client[0];

    std::clock_t start, end;

    // Accept client
    Info other;
    other.len = sizeof(other.addr);
    other.sock = accept(me.sock, (struct sockaddr*)&other.addr, (socklen_t*)&other.len);

    if (other.sock < 0) {
        P_ERROR("Accept error\n");
        exit(1);
    }

    printf("Server ready\n");

    auto test_1 = [&] (const u32 buffer_size) 
    {
        // Test 1: Lots of packets send speed
        buffer = new char[buffer_size];
        u32 resend_needed = 0;

        const u32 max = (u32)std::ceil((double)MEGABYTE / (double)buffer_size);

        start = clock();
        for (u32 i = 0; i < max; i++) {
            i32 send_left = buffer_size;
            i32 sent = 0;
            const char* ptr = buffer;

            while (send_left > 0) {
                sent = send(other.sock, ptr, send_left, 0);

                if (sent < 0) {
                    P_ERROR("Server Lost connection\n");
                    exit(1);
                }
                else if (sent != send_left) {
                    ++resend_needed;
                }

                send_left -= sent;
                ptr += sent;
            }
        }
        end = clock();
        printf(
            "Test 1: Server Report (B Size %u)\n\tSent 1000 packets. (Total %u bytes)\n"
            "\tHad to resend paket a total of '%u' times\n"
            "\tExecution speed: %lf\n\n",
            buffer_size,
            buffer_size * max,
            resend_needed,
            (double)(end - start) / CLOCKS_PER_SEC
        );

        delete buffer;
        // Test 1: End
    };

    test_1(MTU_SIZE);

    s_state.store(true);
    while (c_state.load() != true) ; // Wait
    s_state.store(false);

    test_1(MEGABYTE);
}

void client()
{
    char* buffer;
    const auto& me = ctx.client[1];

    std::clock_t start, end;

    if (connect(me.sock, 
            (struct sockaddr*)&me.addr, 
            sizeof(me.addr)) < 0) {
        P_ERROR("Failed to connect\n");
        exit(1);
    }

    printf("Client ready\n");

    auto test_1 = [&] (const u32 buffer_size) 
    {
        // Test 1
        buffer = new char[buffer_size];
        start = clock();

        const u32 max = (u32)std::ceil((double)MEGABYTE / (double)buffer_size);

        for (u32 i = 0; i < max; i++) {
            i32 r = recv(me.sock, buffer, buffer_size, MSG_WAITALL);

            if (r < 0) {
                P_ERROR("Client Lost connection\n");
                exit(1);
            }
        }

        end = clock();
        printf(
            "Test 1: Client Report (B Size %u)\n"
            "\tExecution speed: %lf\n\n",
            buffer_size,
            (double)(end - start) / CLOCKS_PER_SEC
        );

        delete buffer;
        // Test 1: End
    };

    test_1(MTU_SIZE);

    c_state.store(true);
    while (s_state.load() != true) ;
    c_state.store(false);

    test_1(MEGABYTE);
}

int main() 
{
#ifdef SYSTEM_WIN_64
    if (WSAStartup(MAKEWORD(2,2), &ctx.wsa_data) != 0) {
		P_ERROR("WSAStartup\n");
        exit(1);
    }
#endif

    for (u32 i = 0; i < 2; i++) {
        auto &c = ctx.client[i];

        c.sock = socket(AF_INET, SOCK_STREAM, 0);
        if (c.sock < 0) {
            P_ERROR("Socket Error\n");
            exit(1);
        }

        c.addr.sin_port = htons(8080);
        c.addr.sin_family = AF_INET;
        c.addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        if (i == 0) {
            if (bind(c.sock, (struct sockaddr*)&c.addr, sizeof(c.addr)) < 0) {
                P_ERROR("Cannot bind\n");
                exit(1);
            }

            if (listen(c.sock, SOMAXCONN) < 0) {
                P_ERROR("Listen failed\n");
                exit(1);
            }
        }
    }

    std::thread srv(server);
    sleep(1);
    std::thread cli(client);

    cli.join();
    srv.join();

#ifdef SYSTEM_WIN_64
    closesocket(ctx.client[0].sock);
    closesocket(ctx.client[1].sock);
    WSACleanup();
#elif defined(SYSTEM_UNX)
    close(ctx.client[0].sock);
    close(ctx.client[1].sock);
#endif
}

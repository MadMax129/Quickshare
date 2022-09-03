#include <cstdio>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstdint>
#include <cerrno>
#include <cstring>
#include <assert.h>
#include "../ip_server.hpp"

int main(int argc, const char** argv)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    assert(sock >= 0);

    sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("192.168.1.31");
	addr.sin_port = htons(8080);

    assert(connect(sock, (sockaddr*)&addr, sizeof(addr)) >= 0);

    Ip_Msg msg;
    msg.type = Ip_Msg::REQUEST;

    std::strcpy(msg.request.net_name, "me");
    std::strcpy(msg.request.my_ip, "192.168.1.1");

    assert(send(sock, (char*)&msg, sizeof(msg), 0) == sizeof(Ip_Msg));

    assert(recv(sock, (char*)&msg, sizeof(Ip_Msg), 0) == sizeof(Ip_Msg));

    std::printf("Got response: %s\n Ip: %s\n", 
    msg.type == Ip_Msg::RESPONSE ? "RESPONSE" : "INVALID",
     msg.response.ip);

    close(sock);
}
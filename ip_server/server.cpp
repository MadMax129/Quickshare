#include "ip_server.hpp"
#include "../include/config.hpp"

bool Server::init_server()
{
    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) 
        return false;

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(QS_PUBLIC_IP);
	addr.sin_port = htons(QS_PUBLIC_PORT);

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::printf("Failed to bind '%s':%d\n", QS_PUBLIC_IP, QS_PUBLIC_PORT);
        return false;
    }

    if (listen(sock, SOMAXCONN) < 0)
        return false;

    return true;
}

void Server::loop()
{
    while (global_state.load(std::memory_order_relaxed) == ONLINE) {
        socklen_t len = sizeof(new_addr);
        new_client = accept(sock, (sockaddr*)&new_addr, &len);

        if (new_client < 0) {
            std::printf("Failed to accept client...\n");
            continue;
        }

        std::printf(
            "Accepted '%s':%d\n", 
            inet_ntoa(new_addr.sin_addr), 
            new_addr.sin_port
        );
        
        request();

        close(new_client);
    }
}

static void set_timeout(int sockfd)
{
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
}

void Server::request()
{
    static Ip_Msg msg = {};
    set_timeout(new_client);

    ssize_t to_read = sizeof(Ip_Msg);
    char* buf_ptr = (char*)&msg;

    while (to_read > 0) {
        const ssize_t r_bytes = recv(new_client, buf_ptr, to_read, 0);

        if (r_bytes <= 0) {
            std::printf("Recv failed [%s]\n", std::strerror(errno));
            close(new_client);
            return;
        }

        to_read -= r_bytes;
        buf_ptr += r_bytes;
    }

    handle_msg(msg);
}

void Server::handle_msg(Ip_Msg& msg)
{
    const char* msg_name = "";
    if (msg.type == Ip_Msg::REQUEST) 
        msg_name = "REQUEST";
    else if (msg.type == Ip_Msg::ADD) 
        msg_name = "ADD";

    std::printf(
        "Request:\n"
        "\tType: %s\n"
        "\tName: %s\n"
        "\tIp:   %s\n",
        msg_name,
        msg.request.net_name,
        msg.request.my_ip
    );

    Ip_Msg answer;

    switch (msg.type)
    {
        case Ip_Msg::INVALID:  break;
        case Ip_Msg::RESPONSE: break;

        case Ip_Msg::REQUEST: {
            hosts.find_entry(msg.request.net_name, &answer);
            send_msg(&answer);
            break;
        }

        case Ip_Msg::ADD: {
            hosts.create_entry(msg.request.net_name, msg.request.my_ip, &answer);
            send_msg(&answer);
            break;
        }
    }
}

void Server::send_msg(Ip_Msg* msg)
{
    ssize_t bytes_left = sizeof(Ip_Msg);
    char* buf_ptr = (char*)msg;

    while (bytes_left > 0) {
        const ssize_t s_bytes = send(new_client, buf_ptr, bytes_left, 0);

        if (s_bytes <= 0) {
            std::printf("Failed durin send...\n");
            return;
        }

        bytes_left -= s_bytes;
        buf_ptr    += s_bytes;
    }
}

void Server::end()
{
    close(sock);
}

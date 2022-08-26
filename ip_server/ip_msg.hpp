#pragma once

#include <cstdint>

#define NET_NAME_LEN 15
#define MAX_IP_LEN 16

struct Ip_Msg {
    enum : std::uint8_t {
        INVALID,
        REQUEST,
        RESPONSE,
        ADD,
    } type;

    union {
        struct {
            char net_name[NET_NAME_LEN];
            char my_ip[MAX_IP_LEN];
        } request;

        struct {
            char ip[MAX_IP_LEN];
        } response;
    };
} __attribute__((packed));

static_assert(sizeof(Ip_Msg) == 32);
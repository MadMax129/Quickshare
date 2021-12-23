#pragma once

#include "quickshare.h"

enum Message_Type {
    M_ERROR = 0,
    M_NEW_CLIENT,
    M_GLOBAL_CHAT,
    M_USER_COUNT,
};

struct Intro {
    char username[USERNAME_MAX_LIMIT];
};

struct Chat_Msg {
    unsigned char from_user[USERNAME_MAX_LIMIT];
    unsigned char data[DATA_MAX];
};

struct Tcp_Msg {
    unsigned char m_type;
    union {
        Intro intro;
        Chat_Msg chat;
    } data;
} __attribute__((packed));
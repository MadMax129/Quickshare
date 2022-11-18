#pragma once

#include <cstdint>

/* Max length of unique session key 14 bytes
Example: abcd1234
    - Can contain any ascii character
    - Must be null terminated
*/
#define SESSION_KEY_LEN 15

/* Max length of standard local ip
    255.255.255.255
    - Also null terminated
*/
#define IP_ADDR_LEN 16

struct Ip_Msg 
{
    enum Type : std::uint8_t 
    {
        INVALID  = 255,
        RESPONSE = 254,
        REQUEST  = 253,
        CREATE   = 252
    } type;

    union 
    {
        struct 
        {
            char net_name[SESSION_KEY_LEN];
            char my_ip[IP_ADDR_LEN];
        } request;

        struct 
        {
            char ip[IP_ADDR_LEN];
        } response;
    };
} __attribute__((packed));

static_assert(sizeof(Ip_Msg) == 32);
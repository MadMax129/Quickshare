#include "../../lib/cpp-base64/base64.h"
#include <time.h>
#include <cstring>
#include "util.hpp"

int main()
{
    union {
        struct {
            time_t timestamp;
            char ip[16];
        } e;
        char bytes[sizeof(time_t) + sizeof(e.ip)];
    } key;
    
    key.e.timestamp = time(NULL);
    std::strcpy(key.e.ip, "255.255.255.255");

    // printf("Original String: ");

    // for (int i = 0; i < 24; i++) 
    //     printf("%d ", key.bytes[i]);
    // puts("");

    // printf("Base64 Encoded: ");

    std::string encoded = base64_encode((const unsigned char*)key.bytes, 24);
    
    //printf("%s\n", (encoded.c_str()));
    // printf("Base64 Decoded: ");

    std::string decoded = base64_decode(encoded);
    // for (int i = 0; i < 24; i++)
    //     printf("%d ", (decoded.c_str()[i]));
    // puts("");

    if (!std::strcmp(key.bytes, decoded.c_str())) {
        colored_print(CL_GREEN, "[ OK ] ");
        printf("hash_test.cpp 'Hashes Equal!'\n");
    }
    else {
        colored_print(CL_RED, "[ FAILED ] ");
        printf("hash_test.cpp 'Hashes Not Equal!'\n");
    }
}
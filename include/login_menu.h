#pragma once

#include "networking.h"

struct Context;

struct Login_Menu {
    Login_Menu(Context* context);
    void draw();
    void reset();

private:
    enum {
        L_DEFAULT,
        L_CONNCETING,
        L_FAILED,
        L_CONNECTED
    } sign_in_state, sign_up_state;

    Context* ctx;
    char username[USERNAME_MAX_LIMIT];
    char password[PASSWORD_MAX_LIMIT];
    bool sign_up;
    
    void draw_sign_up();
    inline bool fields_empty(); 
};

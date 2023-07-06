#pragma once

#include "users.hpp"
#include "nfd.h"

class Context;

class Main_Menu {
public:
    enum Transfer_Type {
        T_ERROR,
        T_SENT,
        T_RECV
    };

    Main_Menu(Context& context);
    void draw();

private:
    void add_event(Transfer_Type type, 
                   const char *desc,
                   const char *fname);
    
    void draw_menus();
    void draw_request();
    void draw_session();
    void draw_backlog();
    
    void draw_path();
    const nfdchar_t* open_file();
    
    void draw_users();
    void check_net();
    void read_users();
    void render_users();
    void transfer();

    const nfdchar_t* file_path;
    User_Vec user_list;
    Context& ctx;
};

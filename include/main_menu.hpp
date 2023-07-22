#pragma once

#include "users.hpp"
#include "transfer_manager.hpp"
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
    void render_request();
    void draw_session();
    void render_session();
    void draw_backlog();
    void draw_users();
    void render_users();
    void draw_path();
    
    const nfdchar_t* open_file();
    
    void copy_data();
    void check_net();
    void transfer();

    const char* get_client_name(const Client_ID c_id);

    const nfdchar_t* file_path;
    User_Vec user_list;
    Transfer_Vec transfer_list;
    Context& ctx;
};

#pragma once

#include <array>
#include "net_gui.hpp"

struct Context;

#define BACKLOG_WIN_SIZE ImVec2(120, 250)
#define USERS_WIN_SIZE ImVec2(120, 250)
#define REQUESTS_WIN_SIZE ImVec2(200, 100)

struct Main_Menu {
public:
    enum Transfer_Type {
        T_ERROR,
        T_SENT,
        T_RECV
    };

    Main_Menu(Context* context);
    ~Main_Menu() = default;

    void draw();

private:
    void add_event(Transfer_Type type,  const char *desc,  const char *fname);
    void draw_menus();
    void draw_path();
    void draw_request();
    void draw_session();
    void draw_backlog();
    void draw_users();
    const char* open_file();

    bool open;
    Context* ctx;
    // Msg::Client_List list;
    std::string sSelectedFile;
    std::string sFilePath;
};

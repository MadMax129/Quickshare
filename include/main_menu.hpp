#pragma once

struct Context;

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
};

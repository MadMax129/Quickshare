#pragma once

#include <array>

struct Context;

using Client_Array = std::array<const char*, 3>;

#define MENU_BAR_MARGIN 20
#define BACKLOG_WIN_SIZE ImVec2(120, 250)
#define USERS_WIN_SIZE ImVec2(120, 250)
#define REQUESTS_WIN_SIZE ImVec2(200, 100)

struct Main_Menu {
public:
    enum Transfer_Type {
        Error,
        Sent,
        Incoming
    };

    Main_Menu(Context* context);
    ~Main_Menu() = default;

    void draw();
    void set_state(bool state);
    bool get_state() const;
    void add_clients(Client_Array &clientList);
    bool open_file();
    void add_event(Transfer_Type type,  const char *desc,  const char *fname);
    void incoming_request(const char *desc, const char *fname);

private:
    void draw_path();
    void draw_request();
    void draw_backlog();
    void draw_users();

    bool open;
    Context* ctx;
    std::string sSelectedFile;
    std::string sFilePath;
};

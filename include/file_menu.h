#pragma once

struct Context;

struct File_Menu {
    File_Menu(Context* context);
    ~File_Menu();

    void draw();
    void set_state(bool state);
    bool get_state() const;

private:
    bool open;
    Context* ctx;
};

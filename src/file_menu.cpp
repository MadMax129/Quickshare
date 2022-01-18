#include "gui.h"

File_Menu::File_Menu(Context* context)
{
    ctx = context;
}

File_Menu::~File_Menu()
{

}

void File_Menu::set_state(bool state) 
{
    open = state;
}

bool File_Menu::get_state() const
{
    return open;
}

void File_Menu::draw()
{
    ImGui::Begin("File Sharing", &open);
    ImGui::End();
}




#include "gui.hpp"
#include <array>
#include <windows.h>
#include <string>
#include "nfd.h"

Main_Menu::Main_Menu(Context* context)
{
    ctx = context;
}

void Main_Menu::draw()
{
    ImGui::SetNextWindowPos(ImVec2(0, MENU_BAR_MARGIN));
    ImGui::SetNextWindowSize(
        ImVec2(
            ImGui::GetIO().DisplaySize.x, 
            ImGui::GetIO().DisplaySize.y
        )
    );

    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse  | 
									   ImGuiWindowFlags_NoScrollbar | 
									   ImGuiWindowFlags_NoResize    | 
									   ImGuiWindowFlags_NoMove      |  
									   ImGuiWindowFlags_NoTitleBar;

    if (ImGui::Begin("Main_Menu", NULL, flags)) 
    {
        draw_path();
		draw_request();
		draw_menus();

        ImGui::End();
    }
}

void Main_Menu::draw_path()
{
    const ImVec2 button_size = {
        ImGui::GetWindowSize().x * 0.5f,
        30.0f
    };

	// Center align button
    ImGui::SetCursorPos(
        ImVec2(
            (ImGui::GetIO().DisplaySize.x / 2.0f) - (button_size.x / 2.0f), 
            ImGui::GetCursorPosY() + PATH_BUTTON_MARGIN
        )    
    );

	static char buffer[64] = "Path###Path";

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.44706f, 0.53725f, 0.85490f, 1.0f)); 
    if (ImGui::Button(buffer, button_size)) {
        const char* path = open_file();
		if (path) {
			sprintf(buffer, "%s###Path", path);
			free((void*)path);
		}
    }
    ImGui::PopStyleColor();
}

void Main_Menu::draw_request()
{
	const char* const text = "Incoming Requests";
	ImVec2 req_size = ImVec2(ImGui::GetWindowSize().x * 0.5f, ImGui::GetTextLineHeightWithSpacing() * 3);

	// Center align text
    ImGui::SetCursorPos(
		ImVec2(
			(ImGui::GetWindowSize().x - ImGui::CalcTextSize(text).x) * 0.5f,
			ImGui::GetCursorPosY() + REQUEST_MARGIN
		)
	);
	ImGui::Text(text);

	// Sizing strech Prop FLAG???

	// Center align request box
	ImGui::SetCursorPosX((ImGui::GetWindowSize().x / 2.0f) - (req_size.x / 2.0f));

	constexpr ImGuiTableFlags flags = ImGuiTableFlags_ScrollY      | 
									  ImGuiTableFlags_RowBg        | 
									  ImGuiTableFlags_BordersOuter | 
									  ImGuiTableFlags_BordersV;
		
	if (ImGui::BeginTable("Requests", 3, flags, req_size))
	{
		// TODO: ADD read form queue here 
		for (int row = 0; row < 2; row++)
		{
			ImGui::TableNextRow();
			
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("DESKTOP-2WG534");
			ImGui::TableSetColumnIndex(1);
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0884, 0.680, 0.128, 1.0f));
			ImGui::SmallButton("Accept");
			ImGui::PopStyleColor();
			ImGui::TableSetColumnIndex(2);
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.910, 0.246, 0.246, 1.0f));
			ImGui::SmallButton("Deny");
			ImGui::PopStyleColor();
		}
		ImGui::EndTable();
	}
}

void Main_Menu::draw_menus()
{
	// Align both menus along Y-axis
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + TWO_MENUS_Y_MARGIN);
	const ImVec2 menu_start = ImGui::GetCursorPos();

	draw_backlog();

	// Move back on Y to draw left top/bottom menu
	ImGui::SetCursorPos(menu_start);
	draw_session();
	ImGui::SetCursorPos(menu_start);
	draw_users();
}

void Main_Menu::draw_backlog()
{
	const ImVec2 backlog_size = {
		(ImGui::GetWindowSize().x * 0.5f) - (MENUS_SIDE_MARGIN * 1.5f),
		(ImGui::GetWindowSize().y - MENU_BOTTOM_MARGIN) - 
		(ImGui::GetCursorPosY() + ImGui::GetTextLineHeight())
	};

	// Align text to center of left menu
	ImGui::SetCursorPosX(
		backlog_size.x / 2.0f + MENUS_SIDE_MARGIN - 25.0f
	);
	ImGui::Text("Backlog");

	// Align left menu list box
	ImGui::SetCursorPosX(MENUS_SIDE_MARGIN);

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.32941f, 0.33333f, 0.42353f, 1.0f)); 
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);

	if (ImGui::BeginListBox("##Backlog", backlog_size))
	{
		add_event(T_RECV, "File size: 2000 bytes Sets the list box size based on the number of items that you want to make visible Size default to hold ~7.25 items. We add +25% worth of item height to allow the user to see at a glance if there are more items up/down, without looking at the scrollbar. We don't add this extra bit if items_count <= height_in_items. It is slightly dodgy, because it means a dynamic list of items will make the widget resize occasionally when it crosses that size.", "tsb.png");
		ImGui::EndListBox();
	}
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
}

void Main_Menu::draw_session()
{
	const ImVec2 session_size = {
		(ImGui::GetWindowSize().x * 0.5f) - (MENUS_SIDE_MARGIN * 1.5f), 
		((ImGui::GetWindowSize().y - ImGui::GetCursorPosY()) / 2.0f) - 
		(MENU_BOTTOM_MARGIN * 0.5f) - (ImGui::GetTextLineHeight() * 2.0f)
	};

	// Align text to center or top right menu
	ImGui::SetCursorPosX(
		(session_size.x / 2.0f + (MENUS_SIDE_MARGIN / 2.0f) - 25.0f) + 
		ImGui::GetWindowSize().x / 2.0f
	);
	ImGui::Text("Session");

	// Align top right menu list box
	ImGui::SetCursorPosX((ImGui::GetWindowSize().x / 2.0f) + (MENUS_SIDE_MARGIN * 0.5f));

	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.32941f, 0.33333f, 0.42353f, 1.0f)); 
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);

	if (ImGui::BeginListBox("##Session", session_size))
	{
		ImGui::EndListBox();
	}
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
}

void Main_Menu::draw_users()
{
	const ImVec2 user_size = {
		(ImGui::GetWindowSize().x * 0.5f) - (MENUS_SIDE_MARGIN * 1.5f), 
		((ImGui::GetWindowSize().y - ImGui::GetCursorPosY()) / 2.0f) - 
		(MENU_BOTTOM_MARGIN * 0.5f) - ImGui::GetTextLineHeight()
	};

	// Align users text 
	ImGui::SetCursorPosY(
		ImGui::GetCursorPosY() + 
		((ImGui::GetWindowSize().y - ImGui::GetCursorPosY()) / 2.0f) - 
		(MENU_BOTTOM_MARGIN * 0.5f)
	);

	ImGui::SetCursorPosX(
		(user_size.x / 2.0f + (MENUS_SIDE_MARGIN / 2.0f) - 26.0f) + 
		ImGui::GetWindowSize().x / 2.0f
	);
	ImGui::Text("User List");

	// Align bottom right menu list box
	ImGui::SetCursorPosX((ImGui::GetWindowSize().x / 2.0f) + (MENUS_SIDE_MARGIN * 0.5f));

	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.32941f, 0.33333f, 0.42353f, 1.0f)); 
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);

	if (ImGui::BeginListBox("##Users", user_size))
	{
		ImGui::EndListBox();
	}
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
}

void Main_Menu::add_event(Transfer_Type type, const char *desc, const char *fname)
{
	static bool selectable = false;

	ImGui::Selectable("(?)", &selectable);
	(void)desc;
	// if (selectable)
	// {
	// 	// ImGui::SetNextWindowFocus
	// 	ImGui::Begin("dfs", &selectable);
	// 		ImGui::End();
	// }
	// if (ImGui::BeginPop)

	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetWindowSize().x);
		ImGui::Text(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
	switch(type)
	{
		case T_RECV:
		{
			ImGui::SameLine(0, 5);
			ImGui::TextColored(ImVec4(0.0178, 0.716, 0.890, 1.0f), "Recieved");   
			ImGui::SameLine(0,5);
			ImGui::TextWrapped("'%s'", fname);
			break;               
		}
		case T_SENT:
		{
			ImGui::SameLine(0,5);
			ImGui::TextColored(ImVec4(0.0178, 0.890, 0.105,1.0f), "Sent");
			ImGui::SameLine(0,5);
			ImGui::Text("'%s'", fname);
			break;
		}

		case T_ERROR:
		{
			break;
		}
	}
}

const char* Main_Menu::open_file()
{
    nfdchar_t *outPath = NULL;
    nfdresult_t result = NFD_OpenDialog(NULL, NULL, &outPath);

    if (result == NFD_OKAY)
    {
        puts(outPath);
    }
    else if ( result == NFD_CANCEL )
    {
        puts("User pressed cancel.");
    }
    else 
    {
        printf("Error: %s\n", NFD_GetError() );
    }

    return outPath;
}
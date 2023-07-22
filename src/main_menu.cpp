#include <string>
#include <algorithm>

#include "context.hpp"
#include "util.h"
#include "gui.hpp"

#define MENU_BAR_MARGIN 20.0f
#define PATH_BUTTON_MARGIN 10.0f
#define REQUEST_MARGIN 20.0f
#define TWO_MENUS_Y_MARGIN 20.0f
#define MENUS_SIDE_MARGIN 16.0f
#define MENU_BOTTOM_MARGIN 35.0f

Main_Menu::Main_Menu(Context& context) : ctx(context) {}

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
		// Copy other data
		copy_data();
		
		// Draw path button
        draw_path();

		// Draw request grid
		draw_request();

		// Draw 3 menus
		draw_menus();

		// Check network state
		check_net();

        ImGui::End();
    }
}

void Main_Menu::copy_data()
{
	User_List::get_instance().copy(user_list);
	// Transfer_Manager::get_instance().copy(transfer_list);
}

void Main_Menu::check_net()
{
	// if (ctx.net.state.get() == Network::FAIL_OCCURED) {
	// 	P_ERROR("Network fail occured... Exiting back to login\n");
	// 	ctx.set_appstate(Context::LOGIN);
	// 	thread_manager.close_all();
	// 	ctx.net.reset();
	// }
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
        const nfdchar_t* path = open_file();
		if (path) {
			std::sprintf(buffer, "%.*s###Path", 64 - 8, path);
			if (file_path)
				std::free((void*)file_path);
			file_path = path;
		}
    }
    ImGui::PopStyleColor();
}

void Main_Menu::draw_request()
{
	const char* const text = "Incoming Requests";
	const ImVec2 req_size = {
		ImGui::GetWindowSize().x * 0.8f,
		ImGui::GetTextLineHeightWithSpacing() * 3
	};

	/* Center align text */
	X_CENTER_ALIGN(ImGui::GetWindowSize().x, text);
	SHIFT_VERTICAL(REQUEST_MARGIN);
	ImGui::Text(text);

	// Sizing strech Prop FLAG???

	/* Center align request box */
	ImGui::SetCursorPosX((ImGui::GetWindowSize().x / 2.0f) - 
						 (req_size.x / 2.0f));

	// constexpr ImGuiTableFlags flags = ImGuiTableFlags_ScrollY      | 
	// 								  ImGuiTableFlags_RowBg        | 
	// 								  ImGuiTableFlags_BordersOuter | 
	// 								  ImGuiTableFlags_BordersV;
	
	// ! TABLE SHOULD FIT USER_NAME_LEN CHaracters on left most colum

	if (ImGui::BeginListBox("##Requests", req_size)) {
		render_request();

			// ImGui::Text("DESKTOP-2WG534"); ImGui::SameLine(150);
			// ImGui::Text("'test_file.txt'"); ImGui::SameLine();
			// ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0884, 0.680, 0.128, 1.0f));
			// ImGui::SmallButton("Accept");
			// ImGui::PopStyleColor();
			// ImGui::SameLine();
			// ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.910, 0.246, 0.246, 1.0f));
			// ImGui::SmallButton("Deny");
			// ImGui::PopStyleColor();

		// }

		ImGui::EndListBox();
	}
	// if (ImGui::BeginTable("Requests", 4, flags, req_size)) { 
	// 	for (int row = 0; row < 2; row++)
	// 	{
	// 		ImGui::TableNextRow();
			
	// 		ImGui::TableSetColumnIndex(0);
	// 		ImGui::Text("DESKTOP-2WG534");
	// 		ImGui::TableSetColumnIndex(1);
	// 		ImGui::Text("test.txt");
	// 		ImGui::TableSetColumnIndex(2);
	// 		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0884, 0.680, 0.128, 1.0f));
	// 		ImGui::SmallButton("Accept");
	// 		ImGui::PopStyleColor();
	// 		ImGui::TableSetColumnIndex(3);
	// 		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.910, 0.246, 0.246, 1.0f));
	// 		ImGui::SmallButton("Deny");
	// 		ImGui::PopStyleColor();
	// 	}
	// 	ImGui::EndTable();
	// }
}

const char* Main_Menu::get_client_name(const Client_ID c_id)
{
	const auto user = std::find_if(
		user_list.begin(),
		user_list.end(),
		[&](const User& u) {
			return u.id == c_id;
		}
	);

	return user == user_list.end() ? 
		NULL :
		user->name;
}

void Main_Menu::render_request()
{
	for (const auto& t : Transfer_Manager::get_instance().
							get_recv_transfers()
	) {
		if (t.state.load(std::memory_order_acquire) != 
			Active_Transfer::GET_RESPONSE)
			continue;

		ImGui::Text("%s", get_client_name(t.hdr.from)); 
		ImGui::SameLine(150);
		ImGui::Text("%s", t.file.filename().c_str()); 
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0884, 0.680, 0.128, 1.0f));
		bool accept_btn = ImGui::SmallButton("Accept");
		ImGui::PopStyleColor();
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.910, 0.246, 0.246, 1.0f));
		bool deny_btn = ImGui::SmallButton("Deny");
		ImGui::PopStyleColor();
		if (accept_btn || deny_btn) {
			Transfer_Cmd cmd(
				t.hdr.t_id,
				cmd.d.reply = accept_btn ? 
					1 : 0
			);
			
			Transfer_Manager::get_instance().send_cmd(cmd);
		}
	}
}

void Main_Menu::draw_menus()
{
	/* Align both menus along Y-axis */
	SHIFT_VERTICAL(TWO_MENUS_Y_MARGIN);
	const ImVec2 menu_start = ImGui::GetCursorPos();

	draw_backlog();

	/* Move back on Y to draw left top/bottom menu */
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
		// for (int i = 0; i < 100; i++) {
			// Transfer_Type t = (Transfer_Type)(rand() % 3);
			add_event(T_RECV, "Ftle size: 2000 bytes Sets the list box size based on the number of items that you want to make visible Size default to hold ~7.25 items. We add +25% worth of item height to allow the user to see at a glance if there are more items up/down, without looking at the scrollbar. We don't add this extra bit if items_count <= height_in_items. It is slightly dodgy, because it means a dynamic list of items will make the widget resize occasionally when it crosses that size.", "tsb.png");
			add_event(T_ERROR, "File size: 2000 bytes", "pic.png");
		// }
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

	// Align text to center of top right menu
	ImGui::SetCursorPosX(
		(session_size.x / 2.0f + (MENUS_SIDE_MARGIN / 2.0f) - 25.0f) + 
		ImGui::GetWindowSize().x / 2.0f
	);
	ImGui::Text("Session");

	// Align session list box
	ImGui::SetCursorPosX((ImGui::GetWindowSize().x / 2.0f) + (MENUS_SIDE_MARGIN * 0.5f));

	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.32941f, 0.33333f, 0.42353f, 1.0f)); 
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);

	if (ImGui::BeginListBox("##Session", session_size))
	{
		render_session();
		ImGui::EndListBox();
	}
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
}

static void state_to_text(const Active_Transfer::State state)
{
	const ImVec4 yellow = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
	switch (state)
	{
		case Active_Transfer::SEND_REQ:
		case Active_Transfer::GET_RESPONSE:
			ImGui::TextColored(
				yellow, 
				"Awaiting Approval"
			);
			break;

		case Active_Transfer::ACCEPT:
		case Active_Transfer::DENY:
		case Active_Transfer::PENDING:
			ImGui::TextColored(
				yellow, 
				"Pending"
			);
			break;

		case Active_Transfer::ACTIVE:
			ImGui::TextColored(
				ImVec4(0.0f, 1.0f, 0.0f, 1.0f), 
				"Active"
			);

		case Active_Transfer::EMPTY:
		default:
			break;
	}
}

void Main_Menu::render_session()
{
	Transfer_Manager& t_manager = Transfer_Manager::get_instance();
	auto draw_func = [&](const Transfer_Manager::Transfer_Array& t_array) 
	{
		for (const auto& t : t_array)
		{
			const Active_Transfer::State state = t.state.load(std::memory_order_acquire);
			if (state == Active_Transfer::State::EMPTY)
				continue;
			
			ImGui::Text("%s", t.file.filename().c_str());
			ImGui::SameLine();
		// 	if (t.session) {
		// 		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
		// 		ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
		// 		ImGui::ProgressBar(
		// 			1.0f / 100.0f, 
		// 			ImVec2(70, 20)
		// 		);
		// 		ImGui::PopStyleColor(2);
		// 	}
			state_to_text(state);

			if (&t_array == &t_manager.get_host_transfers()) {
				for (u32 i = 0; i < TRANSFER_CLIENTS_MAX; i++) {
					if (t.hdr.to[i] == 0)
						break;

					ImGui::Text("\t%s", get_client_name(t.hdr.to[i]));
					ImGui::SameLine();
					
					switch (t.accept_list[i])
					{
						case Active_Transfer::EMPTY:
							ImGui::Text("Empty");
							break;
						
						case Active_Transfer::ACCEPT:
							ImGui::Text("Accept");
							break;

						case Active_Transfer::DENY:
							ImGui::Text("Deny");
							break;
						
						default:
						break;
					}
				}
			}
		}
	};

	draw_func(t_manager.get_host_transfers());
	draw_func(t_manager.get_recv_transfers());
}

void Main_Menu::draw_users()
{
	const ImVec2 user_size = {
		(ImGui::GetWindowSize().x * 0.5f) - (MENUS_SIDE_MARGIN * 1.5f), 
		((ImGui::GetWindowSize().y - ImGui::GetCursorPosY()) / 2.0f) - 
		(MENU_BOTTOM_MARGIN * 0.5f) - ImGui::GetTextLineHeight() - 30.0f
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

	/* Align bottom right menu list box */
	ImGui::SetCursorPosX(
		(ImGui::GetWindowSize().x / 2.0f) + 
		(MENUS_SIDE_MARGIN * 0.5f)
	);

	const ImVec2 save_center = {
		ImGui::GetCursorPosX(),
		ImGui::GetCursorPosY() + user_size.y + 10.0f
	};

	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.32941f, 0.33333f, 0.42353f, 1.0f)); 
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);

	if (ImGui::BeginListBox("##Users", user_size))
	{
		render_users();
		ImGui::EndListBox();
	}

	ImGui::SetCursorPos(save_center);

	if (ImGui::Button("Send", {user_size.x, 20.0f}))
		transfer();

	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
}

void Main_Menu::render_users()
{
	u32 count = 0;
	for (auto& c : user_list) 
	{
		char name[PC_NAME_MAX_LEN + 4];
		std::sprintf(
			name, 
			"%s##%d",
			c.name, count
		);
		ImGui::Selectable(name, &c.selected);
		++count;
	}
}

void Main_Menu::transfer()
{
	Transfer_ID t_id[TRANSFER_CLIENTS_MAX] = {};
	u32 count = 0;
	
	for (const auto& c : user_list)
		if (c.selected) 
			t_id[count++] = c.id;
	
	if (!file_path || count == 0)
		return;

	Transfer_Cmd cmd(file_path, t_id);

	assert(Transfer_Manager::get_instance().send_cmd(cmd));

	for (auto& c : user_list)
		c.selected = false;
}

void Main_Menu::add_event(Transfer_Type type, const char *desc, const char *fname)
{
	// static bool selectable = false;

	ImGui::Text("(?)");
	// ImGui::Selectable("(?)", &selectable);
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
		ImGui::Text("%s", desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}

	ImGui::SameLine(0, 5);

	switch(type)
	{
		case T_RECV:
			ImGui::TextColored(ImVec4(0.0178f, 0.716f, 0.890f, 1.0f), "Recieved");   
			break;

		case T_SENT:
			ImGui::TextColored(ImVec4(0.0178f, 0.890f, 0.105f, 1.0f), "Sent");
			break;

		case T_ERROR:
			ImGui::TextColored(ImVec4(0.9f, 0.290f, 0.105f, 1.0f), "Error");
			break;

	}

	ImGui::SameLine(0,5);
	ImGui::TextWrapped("'%s'", fname);
}

const nfdchar_t* Main_Menu::open_file()
{
    nfdchar_t *outPath = NULL;
    nfdresult_t result = NFD_OpenDialog(
		NULL, 
		NULL, 
		&outPath
	);

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
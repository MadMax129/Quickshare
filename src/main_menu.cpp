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
#define MAX_CHARACTERS_IN_SESSIONS 28

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
	Transfer_Manager::get_instance().copy(transfer_list);
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

	/* Center align request box */
	ImGui::SetCursorPosX((ImGui::GetWindowSize().x / 2.0f) - 
						 (req_size.x / 2.0f));

	if (ImGui::BeginListBox("##Requests", req_size)) {
		render_request();
		ImGui::EndListBox();
	}
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
					true : false
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
		render_backlog();
		ImGui::EndListBox();
	}
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
}

void Main_Menu::render_backlog()
{
	for (const auto& t : transfer_list) {
		switch (t.state)
		{
			case Transfer_Info::CANCELLED:
				ImGui::Text("[ Cancelled ] ");
				break;
			
			case Transfer_Info::COMPLETE:
				ImGui::Text("[ Complete ] ");
				break;

			case Transfer_Info::DENIED:
				ImGui::Text("[ Denied ] ");
				break;

			case Transfer_Info::REJECTED:
				ImGui::Text("[ Rejected ] ");
				break;
		}
		ImGui::SameLine();
		ImGui::Text("%s\n", t.file.filename().string().c_str());
	}
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
			break;
		
		case Active_Transfer::CANCEL:
			ImGui::TextColored(
				ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 
				"Cancelled"
			);
			break;

		case Active_Transfer::EMPTY:
		default:
			break;
	}
}

static void progress_bar(const Active_Transfer& transfer)
{
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

	ImGui::ProgressBar(
		(float)transfer.f_manager.get_progress() / 100.0f, 
		ImVec2(70, 20)
	);
	ImGui::PopStyleColor(3);
}

static void cancel_transfer(const Active_Transfer& transfer)
{
	Transfer_Cmd t_cmd(transfer.hdr.t_id);
	assert(Transfer_Manager::get_instance().send_cmd(t_cmd));
}

void Main_Menu::render_session()
{
	Transfer_Manager& t_manager = Transfer_Manager::get_instance();
	auto draw_func = [&](const Transfer_Manager::Transfer_Array& t_array) 
	{
		char btn_name[32];
		u32 count = 0;
		for (const auto& t : t_array)
		{
			const Active_Transfer::State state = t.state.load(
				std::memory_order_acquire
			);

			if (state == Active_Transfer::State::EMPTY || 
				(state == Active_Transfer::GET_RESPONSE && 
				&t_array == &t_manager.get_recv_transfers())
			) {
				continue;
			}

			ImGui::Text(
				t.file.string().length() > MAX_CHARACTERS_IN_SESSIONS ? 
					"%.*s..." : 
					"%.*s",
				MAX_CHARACTERS_IN_SESSIONS,
				t.file.filename().c_str()
			);

			if (state == Active_Transfer::PENDING || 
				state == Active_Transfer::ACTIVE
			) {
				(void)std::sprintf(
					btn_name,
					"Cancel##%u",
					count++
				);

				if (ImGui::Button(btn_name, ImVec2(50, 20)))
					cancel_transfer(t);
				
				ImGui::SameLine();
			}
			progress_bar(t);
			ImGui::SameLine();
			state_to_text(state);
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
#include "login_menu.hpp"
#include "context.hpp"
#include "gui.hpp"
#include <cstring>

#define MAX_INNER_LENGTH 500.0f
#define MAX_INNER_HEIGHT 350.0f
#define INNER_LOGIN_MARGIN 50.0f
#define WELCOME_TEXT_MARGIN 20.0f
#define SESSION_TEXT_MARGIN 3.0f
#define KEY_TEXT_LEFT_MARGIN 50.0f
#define KEY_TEXT_TOP_MARGIN 20.0f
#define ENTER_BUTTON_MARGIN 20.0f
#define ENTER_BUTTON_HEIGHT 30.0f

Login_Menu::Login_Menu(Context& context) : ctx(context)
{
	clean();
}

// void RenderProgressBar(float value, float maxValue)
// {
//     ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
//     ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

//     ImGui::ProgressBar(value / maxValue, ImVec2(200, 10), "Hi");

//     ImGui::PopStyleColor(2);
// }

void Login_Menu::clean()
{
	std::memset(key, 0, sizeof(Key));
	login_state = false;
	error = "";
	state = IDLE;
}

void Login_Menu::draw()
{
	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));    
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

	// Push all colors and style settings...
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.06f, 0.06f, 1.00f));
	ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor(11, 11, 11, 255));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)ImColor(11, 11, 11, 255));
	ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(255, 0, 46));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor(255, 0, 46));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor(255, 0, 46));
	ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(0.37f, 0.37f, 0.37f, 1.00f));
	
	if (ImGui::Begin("Login_Menu", NULL, flags)) 
	{
		draw_inner();

		ImGui::End();
	}

	ImGui::PopStyleColor(7);
}

void Login_Menu::draw_inner()
{
	const float window_x = ImGui::GetWindowSize().x > MAX_INNER_LENGTH ? 
		MAX_INNER_LENGTH : 
		ImGui::GetWindowSize().x;
	
	const float window_y = ImGui::GetWindowSize().y > MAX_INNER_HEIGHT ?
		MAX_INNER_HEIGHT : 
		ImGui::GetWindowSize().y;

	inner_size = {
		window_x - (2.0f * INNER_LOGIN_MARGIN),
		window_y - (2.0f * INNER_LOGIN_MARGIN)
	};

	// Inner square background and border color
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(27.0f/255.0f, 27.0f/255.0f, 27.0f/255.0f, 255));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(27.0f/255.0f, 27.0f/255.0f, 27.0f/255.0f, 255));
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);

	// Align the inner menu to the center of the screen
	ImGui::SetCursorPos(
		ImVec2(
			(ImGui::GetWindowSize().x / 2.0f) - (inner_size.x / 2.0f),
			(ImGui::GetWindowSize().y / 2.0f) - (inner_size.y / 2.0f)
		)
	);

	ImGui::BeginChild("##MainPanel", inner_size, true);
	{
		// Center text
		ImGui::SetCursorPos(
			ImVec2(
				(inner_size.x / 2.0f) - (ImGui::CalcTextSize("Welcome to Quickshare").x / 2.0f), 
				WELCOME_TEXT_MARGIN
			)
		);
		ImGui::Text("Welcome to Quickshare");
		
		X_CENTER_ALIGN(inner_size.x, "Enter a session key");
		ImGui::TextDisabled("Enter a session key");

		// Draw key input box
		draw_key();

		// Draw enter button
		draw_enter();

		// Draw bottom text
		draw_text();

		// State machine
		net_check();

		ImGui::EndChild();
	}

	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(1);
}

void Login_Menu::draw_key()
{
	ImGui::PushItemWidth(
		inner_size.x - 
		KEY_TEXT_LEFT_MARGIN - 
		KEY_TEXT_LEFT_MARGIN
	);
		
	ImGui::SetCursorPos(
		ImVec2(
			KEY_TEXT_LEFT_MARGIN,
			ImGui::GetCursorPosY() + KEY_TEXT_TOP_MARGIN
		)
	);
	ImGui::TextDisabled("Session Key");

	ImGui::SetCursorPosX(KEY_TEXT_LEFT_MARGIN);
	ImGui::InputText("##Key_Id", key, IM_ARRAYSIZE(key), ImGuiInputTextFlags_CharsNoBlank);
	
	ImGui::PopItemWidth();
}

void Login_Menu::button()
{
	if (strnlen(key, IM_ARRAYSIZE(key) - 1) > 0) {
		Network::get_instance().session(ctx.get_name(), key, login_state);
	}
}

void Login_Menu::draw_enter()
{
	const char* button_text;

	if (!login_state)
		button_text = "Enter";
	else
		button_text = "Create";

	ImGui::SetCursorPos(
		ImVec2(
			KEY_TEXT_LEFT_MARGIN, // Same margin as key
			ImGui::GetCursorPosY() + ENTER_BUTTON_MARGIN
		)
	);

	const ImVec2 enter_size = {
		inner_size.x - KEY_TEXT_LEFT_MARGIN - KEY_TEXT_LEFT_MARGIN,
		ENTER_BUTTON_HEIGHT
	};

	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.f);
	if (ImGui::Button(button_text, enter_size))
		button();
	ImGui::PopStyleVar();
}

void Login_Menu::net_check()
{
	switch (Network::get_instance().get_state())
	{
		case Network::State::INIT_ERROR:
		case Network::State::NET_ERROR:
			state = NETWORK_ERROR;
			error = "Network failed to initiate...";
			break;

		case Network::State::SESSION_ERROR:
			state = NETWORK_ERROR;
			error = "Session ID is NOT valid...";
			break;

		case Network::State::CONNECTED:
		case Network::State::OPENED:
			state = CONNECTING;
			error = "Conecting...";
			break;

		case Network::State::SESSION_SUCCESS:
			ctx.set_appstate(Context::State::MAIN_MENU);
			break;
			
		default:
			break;
	}
}
		
void Login_Menu::draw_text()
{
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(27.0f/255.0f, 27.0f/255.0f, 27.0f/255.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(27.0f/255.0f, 27.0f/255.0f, 27.0f/255.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(27.0f/255.0f, 27.0f/255.0f, 27.0f/255.0f, 255));

	const char* text = login_state ? 
		"Already have a key? Enter it!" :
		"Don't have a key? Create one!";

	X_CENTER_ALIGN(inner_size.x, text);
	SHIFT_VERTICAL(10.0f);
	if (ImGui::SmallButton(text))
		login_state = !login_state;

	ImGui::PopStyleColor(3);

	X_CENTER_ALIGN(inner_size.x, error);
	SHIFT_VERTICAL(10.0f);
	if (state == NETWORK_ERROR) 
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", error);
	else if (state == CONNECTING)
		// ImGui::Text("Connecting %*s", 7, anim[(int)(ImGui::GetTime() / 0.3f) % 7]);
		ImGui::TextColored(ImVec4(146/255.f, 230/255.f, 147/255.f, 1.0f), "Connecting %c", ".oOOo."[(int)(ImGui::GetTime() / 0.3f) % 6]);
		// ImGui::Text("Connecting %c", "|/-\\"[(int)(ImGui::GetTime() / 0.3f) & 3]);
	else
		ImGui::Text(" ");

}
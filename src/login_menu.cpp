#include "login_menu.hpp"
#include "config.hpp"
#include "context.hpp"
#include "gui.hpp"
#include <cstring>

Login_Menu::Login_Menu(Context& context) : ctx(context)
{
	clean();
}

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
		
		ImGui::SetCursorPosX(
			(inner_size.x / 2.0f) - 
			(ImGui::CalcTextSize("Enter a session key").x / 2.0f)
		);
		ImGui::TextDisabled("Enter a session key");

		// Draw key input box
		draw_key();

		// Draw enter button
		draw_enter();

		// Draw bottom text
		draw_text();

		// State machine
		check_state();

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
	{
		if (strnlen(key, IM_ARRAYSIZE(key) - 1) > 0) {
			if (state == IDLE) {
				if (!std::strcmp(key, "test")) {
					// ! TEST KEY
					ctx.set_appstate(Context::MAIN_MENU);
					goto end;
				}
				// ctx.loc.start(
				// 	login_state ? 
				// 	Locator::Mode::CREATE : 
				// 	Locator::Mode::LOCATE,
				// 	key			  
				// );
				state = LOCATOR;
			}
		}
	}
end:
	ImGui::PopStyleVar();
}

void Login_Menu::loc_check()
{
	// switch (ctx.loc.state.get(std::memory_order_acquire))
	// {
	// 	case Locator::INACTIVE:
	// 	case Locator::WORKING:
	// 		error = "";
	// 		break;

	// 	case Locator::CONN_FAILED:
	// 	case Locator::FAILED:
	// 		error = "Error Invalid Key (expired)...";
	// 		/* Move state back to idle, do not start network */
	// 		state = IDLE;
	// 		break;

	// 	case Locator::SUCCESS:
	// 		ctx.net.init_network(ctx.loc.get_ip());
	// 		/* Reset the locator */
	// 		ctx.loc.reset();
	// 		state = NETWORK;
	// 		break;
	// }
}

void Login_Menu::net_check()
{
	// switch (ctx.net.state.get())
	// {
	// 	case Network::State::INACTIVE:
	// 		error = "";
	// 		break;

	// 	case Network::State::INIT_FAILED:
	// 	case Network::State::FAIL_OCCURED:
	// 		error = "Cannot connect to host... Try again...";
	// 		state = IDLE;
	// 		break;

	// 	case Network::State::SUCCESS:
	// 		ctx.set_appstate(Context::MAIN_MENU);
	// 		break;
	// }
}

void Login_Menu::check_state()
{
	switch (state)
	{
		case IDLE: break;
		case LOCATOR: loc_check(); break;
		case NETWORK: net_check(); break;
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

	// Error message
	X_CENTER_ALIGN(inner_size.x, error);
	SHIFT_VERTICAL(10.0f);
	ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", error);

}
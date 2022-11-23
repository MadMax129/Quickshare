#pragma once

#define IMGUI_NEW_FRAME() \
    ImGui_ImplOpenGL3_NewFrame(); \
    ImGui_ImplGlfw_NewFrame(); \
    ImGui::NewFrame();

#define IMGUI_CLEANUP() \
    ImGui_ImplOpenGL3_Shutdown(); \
    ImGui_ImplGlfw_Shutdown(); \
    ImGui::DestroyContext();

#define X_CENTER_ALIGN(len, text) \
    do { \
        ImGui::SetCursorPosX((len / 2.0f) - (ImGui::CalcTextSize(text).x / 2.0f)); \
    } while (0);

#define SHIFT_VERTICAL(dir) \
    do { \
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + dir); \
    } while (0);

#define SHIFT_HORIZONTAL(dir) \
    do { \
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + dir); \
    } while (0);

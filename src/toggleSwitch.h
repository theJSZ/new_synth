#include "imgui/imgui.h"

bool ToggleSwitch(const char* label, bool* v)
{
    float height = ImGui::GetFrameHeight();
    float width = height * 1.5f; // Make the toggle slightly wider
    float radius = height * 0.5f;

    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Background
    ImU32 bg_col = IM_COL32(10, 10, 10, 255);
    draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), bg_col, radius);

    // Knob
    float knob_x = *v ? (p.x + width - radius - 9.0) : (p.x);
    draw_list->AddCircleFilled(ImVec2(knob_x + radius, p.y + radius), radius - 2.0f, IM_COL32(255, 255, 255, 255));

    // Handle input
    ImGui::InvisibleButton(label, ImVec2(width, height));
    if (ImGui::IsItemClicked())
    {
        *v = !*v; // Toggle the value
        return true;
    }

    return false;
}

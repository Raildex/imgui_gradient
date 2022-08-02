#include <imgui_gradient/imgui_gradient.hpp>
#include <iterator>
#include "gradient_widgets.h"
#include "imgui_draw.h"
#include "random.h"

namespace ImGuiGradient {

static void draw_gradient_bar(Gradient& Gradient, const Interpolation& interpolation_mode, const ImVec2& bar_pos, float width, float height)
{
    ImDrawList& draw_list  = *ImGui::GetWindowDrawList();
    const float bar_bottom = bar_pos.y + height;

    draw_bar_border(draw_list, bar_pos, ImVec2(bar_pos.x + width, bar_bottom), variables::border_color());
    if (!Gradient.is_empty())
    {
        draw_gradient(Gradient, draw_list, interpolation_mode, bar_pos, bar_bottom, width);
    }

    ImGui::SetCursorScreenPos(ImVec2(bar_pos.x, bar_pos.y + height + 10.0f));
}

static bool mark_hovered(Mark*& dragging_mark, Mark*& selected_mark, Mark*& mark_to_delete, Mark& mark)
{
    bool is_hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    if (is_hovered)
    {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            dragging_mark = &mark;
        }
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) ||
            ImGui::IsMouseDoubleClicked(ImGuiPopupFlags_MouseButtonLeft))
        {
            selected_mark = &mark;
        }
        if (ImGui::IsMouseDoubleClicked(ImGuiPopupFlags_MouseButtonLeft))
        {
            ImGui::OpenPopup("picker");
        }
        if (ImGui::IsMouseReleased(ImGuiPopupFlags_MouseButtonMiddle))
        {
            // When we middle click to delete a non selected mark it is impossible to remove this mark in the loop
            mark_to_delete = &mark;
        }
    }
    return is_hovered;
}

static bool draw_gradient_marks(Gradient& gradient, Mark*& dragging_mark, Mark*& selected_mark, Mark*& mark_to_delete, Mark*& mark_to_hide, const ImVec2& bar_pos, float width, float height)
{
    ImDrawList& draw_list         = *ImGui::GetWindowDrawList();
    bool        hitbox_is_hovered = false;
    for (auto markIt = gradient.get_list().begin(); markIt != gradient.get_list().end(); ++markIt)
    {
        Mark& mark = *markIt;
        if (!(mark_to_hide && *mark_to_hide == mark))
        {
            mark_button(
                draw_list,
                bar_pos + ImVec2(mark.get_position() * width, height),
                ImGui::ColorConvertFloat4ToU32(mark.color),
                selected_mark == &mark
            );
        }
        hitbox_is_hovered |= mark_hovered(dragging_mark, selected_mark, mark_to_delete, mark);
    }
    ImGui::SetCursorScreenPos(ImVec2(bar_pos.x, bar_pos.y + height + 20.0f));
    return hitbox_is_hovered;
}

bool gradient_button(Gradient& gradient, const Interpolation& interpolation)
{
    const ImVec2 widget_pos = ImGui::GetCursorScreenPos();
    const float  width      = ImMax(250.0f, ImGui::GetContentRegionAvail().x - 100.0f);
    const bool   clicked    = ImGui::InvisibleButton("gradient_bar", ImVec2(widget_pos.x + width, variables::GRADIENT_BAR_WIDGET_HEIGHT));

    draw_gradient_bar(gradient, interpolation, widget_pos, width, variables::GRADIENT_BAR_WIDGET_HEIGHT);

    return clicked;
}

float position_where_add_mark(Gradient& gradient)
{
    if (gradient.is_empty())
    {
        return 0.5f;
    }
    else if (gradient.get_list().size() == 1)
    {
        return (gradient.get_list().begin()->get_position() > 1.f - gradient.get_list().begin()->get_position()) ? 0.f : 1.f;
    }
    else
    {
        float max_value_mark_position     = 0;
        float max_value_between_two_marks = gradient.get_list().begin()->get_position();
        for (auto markIt = gradient.get_list().begin(); markIt != std::prev(gradient.get_list().end()); ++markIt)
        {
            Mark& mark = *markIt;
            if (max_value_between_two_marks < abs(std::next(markIt)->get_position() - mark.get_position()))
            {
                max_value_mark_position     = mark.get_position();
                max_value_between_two_marks = abs(std::next(markIt)->get_position() - max_value_mark_position);
            }
        }
        if (max_value_between_two_marks < abs(1.f - std::prev(gradient.get_list().end())->get_position()))
        {
            max_value_mark_position     = std::prev(gradient.get_list().end())->get_position();
            max_value_between_two_marks = abs(1.f - max_value_mark_position);
        }
        return max_value_mark_position + max_value_between_two_marks / 2.f;
    }
}

auto GradientWidget::mouse_dragging(const float bar_bottom, float width, float bar_pos_x, GradientOptions options) -> bool
{
    bool dragging = false;
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) && dragging_mark)
    {
        dragging_mark = nullptr;
    }

    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && dragging_mark)
    {
        const float map = ImClamp((ImGui::GetIO().MousePos.x - bar_pos_x) / width, 0.f, 1.f);
        if (dragging_mark->get_position() != map)
        {
            dragging_mark->position.set(map);
            gradient.get_marks().sorted();
            dragging = true;
        }
        if (!(options & GradientOptions_NoDragDowntoDelete))
        { // hide dragging mark when mouse under gradient bar
            float diffY = ImGui::GetIO().MousePos.y - bar_bottom;
            if (diffY >= variables::GRADIENT_MARK_DELETE_DIFFY)
            {
                mark_to_hide = dragging_mark;
            }
            // do not hide it anymore when mouse on gradient bar
            if (mark_to_hide && diffY <= variables::GRADIENT_MARK_DELETE_DIFFY)
            {
                dragging_mark = mark_to_hide;
                mark_to_hide  = nullptr;
            }
        }
    }
    return dragging;
}

bool GradientWidget::gradient_editor(std::string_view name, std::default_random_engine& generator, float horizontal_margin, GradientOptions options, ImGuiColorEditFlags flags)
{
    if (!(options & GradientOptions_NoLabel))
    {
        ImGui::Text("%s", name.data());
        ImGui::Dummy(ImVec2{0.f, 1.5f});
    }

    const ImVec2 bar_pos    = variables::bar_position(horizontal_margin);
    const float  width      = std::max(1.f, ImGui::GetContentRegionAvail().x - 2.f * horizontal_margin);
    const float  bar_bottom = bar_pos.y + variables::GRADIENT_BAR_EDITOR_HEIGHT;

    ImGui::BeginGroup();
    ImGui::InvisibleButton("gradient_editor_bar", ImVec2(width, variables::GRADIENT_BAR_EDITOR_HEIGHT));
    draw_gradient_bar(gradient, interpolation_mode, bar_pos, width, variables::GRADIENT_BAR_EDITOR_HEIGHT);

    Mark*      mark_to_delete         = nullptr;
    const bool add_mark_possible      = ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    const bool mark_hitbox_is_hovered = draw_gradient_marks(gradient, dragging_mark, selected_mark, mark_to_delete, mark_to_hide, bar_pos, width, variables::GRADIENT_BAR_EDITOR_HEIGHT);

    bool modified = false;
    if (add_mark_possible && !mark_hitbox_is_hovered)
    {
        modified = add_mark((ImGui::GetIO().MousePos.x - bar_pos.x) / width, generator);
        ImGui::OpenPopup("picker");
    }

    modified |= mouse_dragging(bar_bottom, width, bar_pos.x, options);
    if (!(options & GradientOptions_NoDragDowntoDelete))
    { // If mouse released and there is still a mark hidden, then it become a mark to delete
        if (mark_to_hide && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            if (dragging_mark && *dragging_mark == *mark_to_hide)
            {
                dragging_mark = nullptr;
            }
            mark_to_delete = mark_to_hide;
            mark_to_hide   = nullptr;
            modified |= true;
        }
    }
    // Remove mark_to_delete if it exists
    if (mark_to_delete)
    {
        if (selected_mark && *selected_mark == *mark_to_delete)
        {
            selected_mark = nullptr;
        }
        gradient.remove_mark(*mark_to_delete);
        modified |= true;
    }
    ImGui::EndGroup();

    const bool remove_button_exists = !(options & GradientOptions_NoRemoveButton);
    if (!gradient.is_empty())
    {
        if (((remove_button_exists &&
              delete_button(variables::button_size(), options)) ||
             ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace)) &&
            selected_mark)
        {
            gradient.remove_mark(*selected_mark);
            selected_mark = nullptr;
            modified |= true;
        }
    }
    const bool add_button_exists = !(options & GradientOptions_NoAddButton);
    if (add_button_exists)
    {
        if (remove_button_exists && !gradient.is_empty())
        {
            ImGui::SameLine();
        }
        if (add_button(variables::button_size(), options))
        {
            // Add a mark where there is the greater space in the gradient
            modified = add_mark(position_where_add_mark(gradient), generator);
        }
    }
    const bool color_edit_exists = !(options & GradientOptions_NoColorEdit);
    if (color_edit_exists)
    {
        if ((remove_button_exists || add_button_exists) && selected_mark)
        {
            ImGui::SameLine();
        }
        modified |= color_button(selected_mark, options, flags);
    }
    if (!(options & GradientOptions_NoDragSlider))
    {
        if ((remove_button_exists || add_button_exists || color_edit_exists) && selected_mark)
        {
            ImGui::SameLine();
        }

        modified |= precise_position(gradient, selected_mark, width, options);
    }

    const bool interpolation_combo_exists = !(options & GradientOptions_NoInterpolationCombo);
    if (interpolation_combo_exists)
    {
        modified |= gradient_interpolation_mode(interpolation_mode);
    }
    const bool position_mode_combo_exists = !(options & GradientOptions_NoPositionModeCombo);
    if (position_mode_combo_exists)
    {
        if (interpolation_combo_exists)
        {
            ImGui::SameLine();
        }
        modified |= position_mode_combo(position_mode);
    }

    if (!(options & GradientOptions_NoRandomMode))
    {
        if (position_mode_combo_exists || interpolation_combo_exists)
        {
            ImGui::SameLine();
        }
        modified |= random_mode_box(random_mode, options);
    }

    if (!(options & GradientOptions_NoResetButton))
    {
        if (reset_button(options))
        {
            reset_widget();
            modified |= true;
        }
    }

    modified |= popup(selected_mark, variables::button_size(), options, flags);

    if (!(options & GradientOptions_NoBorder))
    {
        float y_space_over_bar = 8.f;
        if (!(options & GradientOptions_NoLabel))
        {
            y_space_over_bar = ImGui::CalcTextSize(name.data()).y * 2.3f;
        }
        float number_of_line_under_bar = 0.f;
        if (!(options & GradientOptions_NoComboAndRandomMode))
        {
            number_of_line_under_bar += 1.f;
        }
        if (!(options & GradientOptions_NoResetButton))
        {
            number_of_line_under_bar += 1.f;
        }
        if (!(options & GradientOptions_NoButtonAndMarkOptions))
        {
            number_of_line_under_bar += 1.f;
        }
        const float y_space_under_bar = bar_bottom + variables::button_size() * number_of_line_under_bar;
        draw_border_widget(
            bar_pos - ImVec2(horizontal_margin + 4.f, y_space_over_bar),
            ImVec2(bar_pos.x + width + horizontal_margin + 4.f, y_space_under_bar * 1.25f),
            variables::border_color()
        );
    }

    return modified;
}

}; // namespace ImGuiGradient
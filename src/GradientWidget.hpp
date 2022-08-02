// https://gist.github.com/galloscript/8a5d179e432e062550972afcd1ecf112

/* TODO(ASG) Replace this with our own documentation

 Usage:

 ::GRADIENT DATA::
 ImGradient gradient;

 ::BUTTON::
 if(ImGui::GradientButton(&gradient))
 {
    //set show editor flag to true/false
 }

 ::EDITOR::
 static ImGradientMark* draggingMark = nullptr;
 static ImGradientMark* selectedMark = nullptr;

 bool updated = ImGui::GradientEditor(&gradient, draggingMark, selectedMark);

 ::GET A COLOR::
 float color[3];
 gradient.getColorAt(0.3f, color); //position from 0 to 1

 ::MODIFY GRADIENT WITH CODE::
 gradient.get().clear();
 gradient.add_mark(0.0f, ImColor(0.2f, 0.1f, 0.0f));
 gradient.add_mark(0.7f, ImColor(120, 200, 255));

 ::WOOD BROWNS PRESET::
 gradient.get().clear();
 gradient.add_mark(0.0f, ImColor(0xA0, 0x79, 0x3D));
 gradient.add_mark(0.2f, ImColor(0xAA, 0x83, 0x47));
 gradient.add_mark(0.3f, ImColor(0xB4, 0x8D, 0x51));
 gradient.add_mark(0.4f, ImColor(0xBE, 0x97, 0x5B));
 gradient.add_mark(0.6f, ImColor(0xC8, 0xA1, 0x65));
 gradient.add_mark(0.7f, ImColor(0xD2, 0xAB, 0x6F));
 gradient.add_mark(0.8f, ImColor(0xDC, 0xB5, 0x79));
 gradient.add_mark(1.0f, ImColor(0xE6, 0xBF, 0x83));

 */

#pragma once

#include "Gradient.h"
#include "GradientOptions.h"
#include "Interpolation.h"
#include "random.h"


namespace ImGuiGradient {

static auto random_color(std::default_random_engine& generator) -> ImVec4
{
    const auto color = ImVec4{utils::rand(generator), utils::rand(generator), utils::rand(generator), 1.f};
    return color;
}

class GradientWidget {
public:
    GradientWidget() = default;
    const Gradient& get_gradient() const { return gradient; }
    Gradient&       get_gradient() { return gradient; }
    bool            add_mark(const float position, std::default_random_engine& generator)
    {
        const float  pos          = ImClamp(position, 0.f, 1.f);
        const ImVec4 new_mark_col = (random_mode) ? random_color(generator) : gradient.get_color_at(pos, position_mode);
        return (selected_mark = gradient.add_mark(Mark{pos, new_mark_col}));
    }
    void reset_widget()
    {
        gradient.reset();
        dragging_mark      = nullptr;
        selected_mark      = nullptr;
        mark_to_hide       = nullptr;
        position_mode      = PositionMode::clamp;
        interpolation_mode = Interpolation::linear;
        random_mode        = false;
    }
    auto mouse_dragging(const float bar_bottom, float width, float bar_pos_x, GradientOptions options = GradientOptions_None) -> bool;
    bool gradient_editor(std::string_view name, std::default_random_engine& generator, float horizontal_margin = 10.f, GradientOptions options = GradientOptions_None, ImGuiColorEditFlags flags = 0);

private:
    Gradient      gradient{};
    Mark*         dragging_mark{};
    Mark*         selected_mark{};
    Mark*         mark_to_hide{};
    PositionMode  position_mode      = PositionMode::clamp;
    Interpolation interpolation_mode = Interpolation::linear;
    bool          random_mode        = false;
};

bool gradient_button(Gradient* gradient, const Interpolation& interpolation);

} // namespace ImGuiGradient
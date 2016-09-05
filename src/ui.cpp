#include "ui.h"

#include "../easyloggingpp/easylogging++.h"

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

using namespace std;

#include "../stb/stb_easy_font.h"

void MouseClickHandler::update_mouse_state(MouseButton button, MouseState state)
{
    auto now = chrono::high_resolution_clock::now();
    auto curr = _state[button];
    if (curr != state)
    {
        auto ms = chrono::duration_cast<chrono::milliseconds>
                        (now - _last_update[button]).count();
        if (ms < CLICK_TIME_MS)
        {
            if (state == MouseState::up)
            {
                ms = chrono::duration_cast<chrono::milliseconds>
                     (now - _last_click[button]).count();
                _last_click[button] = now;
                
                if (is_enabled())
                {
                    if (ms < CLICK_TIME_MS && button == MouseButton::left) {
                        _on_double_click();
                    } else {
                        _on_click[button]();
                    }
                }
            }
        }

        _state[button] = state;
    }
    _last_update[button] = now;
}

MouseClickHandler::MouseClickHandler()
    : _on_double_click([this](){ _on_click[MouseButton::left](); })
{
    _state[MouseButton::left] = _state[MouseButton::right] =
    _state[MouseButton::middle] = MouseState::up;

    _on_click[MouseButton::left] = _on_click[MouseButton::right] =
    _on_click[MouseButton::middle] = [](){};

    auto now = chrono::high_resolution_clock::now();
    _last_update[MouseButton::left] = _last_update[MouseButton::right] =
    _last_update[MouseButton::middle] = now;

    _last_click[MouseButton::left] = _last_click[MouseButton::right] =
    _last_click[MouseButton::middle] = now;
}

Rect ControlBase::arrange(const Rect& origin)
{
    auto x0 = _position.x.to_pixels(origin.size.x);
    auto y0 = _position.y.to_pixels(origin.size.y);

    auto size = get_size();
    auto w = min(size.x.to_pixels(origin.size.x), origin.size.x);
    auto h = min(size.y.to_pixels(origin.size.y), origin.size.y);

    y0 += origin.position.y; // Y is unaffected by alignment for now
    // TODO: vertical alignment? is it useful?
    
    if (_align == Alignment::left)
    {
        x0 += origin.position.x;
    }
    else if (_align == Alignment::right)
    {
        x0 = origin.position.x + origin.size.x - x0 - w;
    }
    else // center
    {
        x0 = origin.position.x + origin.size.x / 2 - w / 2;
    }

    return { { x0, y0 }, { w, h } };
}

Size2 ControlBase::get_size() const
{
    Size x = _size.x.is_auto() ? get_intrinsic_size().x : _size.x;
    Size y = _size.y.is_auto() ? get_intrinsic_size().y : _size.y;
    return { x, y };
}

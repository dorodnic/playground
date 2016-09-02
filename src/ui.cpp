#include "ui.h"

#include "../easyloggingpp/easylogging++.h"

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

using namespace std;

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

                if (ms < CLICK_TIME_MS && button == MouseButton::left) {
                    _on_double_click();
                } else {
                    _on_click[button]();
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
    auto x0 = _position.x.to_pixels(origin.size.x) + _margin.left;
    auto y0 = _position.y.to_pixels(origin.size.y) + _margin.top;

    auto size = get_size();
    auto w = min(size.x.to_pixels(origin.size.x),
                      origin.size.x - _margin.right - _margin.left);
    auto h = min(size.y.to_pixels(origin.size.y),
                      origin.size.y - _margin.bottom - _margin.top);

    x0 += origin.position.x;
    y0 += origin.position.y;

    return { { x0, y0 }, { w, h } };
}

Size2 ControlBase::get_size() const
{
    Size x = _size.x.is_auto() ? get_intrinsic_size().x : _size.x;
    Size y = _size.y.is_auto() ? get_intrinsic_size().y : _size.y;
    return { x, y };
}

void Button::render(const Rect& origin)
{
    glBegin(GL_QUADS);

    auto c = _color;
    if (is_focused()) c = brighten(c, 1.1f);
    if (is_pressed(MouseButton::left)) c = brighten(c, 0.8f);

    glColor3f(c.r, c.g, c.b);

    auto rect = arrange(origin);

    glVertex2i(rect.position.x, rect.position.y);
    glVertex2i(rect.position.x, rect.position.y + rect.size.y);
    glVertex2i(rect.position.x + rect.size.x,
               rect.position.y + rect.size.y);
    glVertex2i(rect.position.x + rect.size.x,
               rect.position.y);

    glEnd();
}

void Container::update_mouse_position(Int2 cursor)
{
    Size Size2::* field;
    int Int2::* ifield;
    if (_orientation == Orientation::vertical) {
        field = &Size2::y;
        ifield = &Int2::y;
    } else {
        field = &Size2::x;
        ifield = &Int2::x;
    }

    auto sum = _arrangement.position.*ifield;
    _focused = nullptr;
    for (auto& p : _content) {
        auto new_origin = _arrangement;
        (new_origin.position.*ifield) = sum;
        (new_origin.size.*ifield) = _sizes[p.get()].first;
        sum += _sizes[p.get()].first;

        //cout << new_origin << " - " << cursor << endl;

        if (contains(new_origin, cursor))
        {
            _focused = p.get();
            p->focus(true);
            p->update_mouse_position(cursor);
        }
        else
        {
            p->focus(false);
        }
    }
}

void Container::add_item(shared_ptr<IVisualElement> item)
{
    invalidate_layout();

    auto container = dynamic_cast<Container*>(item.get());
    if (container)
    {
        container->set_items_change([this](){
            invalidate_layout();
        });
    }

    _on_items_change();

    _content.push_back(move(item));
}

SizeMap Container::calc_sizes(Orientation orientation,
                              const Elements& content,
                              const Rect& arrangement)
{
    SizeMap sizes;

    auto sum = 0;

    Size Size2::* field;
    int Int2::* ifield;
    Size Size2::* other;
    if (orientation == Orientation::vertical) {
        field = &Size2::y;
        ifield = &Int2::y;
        other = &Size2::x;
    } else {
        field = &Size2::x;
        ifield = &Int2::x;
        other = &Size2::y;
    }

    // first, scan items, map the "greedy" ones wanting relative portion
    vector<IVisualElement*> greedy;
    for (auto& p : content) {
        auto p_rect = p->arrange(arrangement);
        auto p_total = p->get_margin().apply(p_rect);
        auto p_size = p->get_size();

        if ((p_size.*field).is_const()) {
            auto pixels = (p_size.*field).get_pixels();
            sum += pixels;
            sizes[p.get()].first = pixels;
            sizes[p.get()].second = p_size.*other;
        } else {
            greedy.push_back(p.get());
        }
    }

    auto rest = max(arrangement.size.*ifield - sum, 0);
    float total_parts = 0;
    for (auto ptr : greedy) {
        total_parts += (ptr->get_size().*field).get_percents();
    }
    for (auto ptr : greedy) {
        auto f = ((ptr->get_size().*field).get_percents() / total_parts);
        sizes[ptr].first = (int) (rest * f);
        sizes[ptr].second = ptr->get_size().*other;
    }

    return sizes;
}

Size2 Container::get_intrinsic_size() const
{
    auto sizes = calc_global_sizes({ { 0, 0 }, { 0, 0 } });
    auto total = 0;
    auto max = 0;
    for (auto& kvp : sizes) {
        auto pixels = kvp.second.first;
        auto size = kvp.second.second;

        total += pixels;
        max = std::max(max, size.to_pixels(0));
    }

    if (_orientation == Orientation::vertical) {
        return { Size(max), Size(total) };
    } else {
        return { Size(total), Size(max) };
    }
};

void Container::render(const Rect& origin)
{
    Size Size2::* field;
    int Int2::* ifield;
    if (_orientation == Orientation::vertical) {
        field = &Size2::y;
        ifield = &Int2::y;
    } else {
        field = &Size2::x;
        ifield = &Int2::x;
    }

    if (!(_origin == origin))
    {
        _arrangement = arrange(origin);
        _sizes = calc_global_sizes(_arrangement);
        _origin = origin;
    }

    auto sum = _arrangement.position.*ifield;
    for (auto& p : _content) {
        auto new_origin = _arrangement;
        (new_origin.position.*ifield) = sum;
        (new_origin.size.*ifield) = _sizes[p.get()].first;
        sum += _sizes[p.get()].first;
        p->render(new_origin);
    }
}

void Grid::commit_line()
{
    if (_current_line) {
        Container::add_item(_current_line);
        _lines.push_back(_current_line);
    }
    _current_line = shared_ptr<Container>(
        new Container({0,0}, { 1.0f, 1.0f }, 0,
        get_orientation() == Orientation::vertical 
            ? Orientation::horizontal : Orientation::vertical,
        this));
}

SizeMap Grid::calc_sizes(const Container* sender,
                         const Rect& arrangement) const
{
    map<const Container*, SizeMap> maps;
    auto max_length = 0;
    for (auto& p : _lines)
    {
        auto map = p->calc_local_sizes(arrangement);
        maps[p.get()] = map;
        max_length = max(max_length, (int)map.size());
    }
    
    vector<vector<int>> grid;
    for (auto& p : _lines)
    {
        vector<int> row(max_length, 0);
        auto i = 0;
        for (auto& kvp : maps[p.get()])
        {
            row[i++] = kvp.second.first;
        }
        grid.push_back(row);
    }
    
    auto result = maps[sender];
    
    auto _index = 0;
    for (auto& kvp : result)
    {
        auto max = 0;
        for (auto j = 0; j < grid.size(); j++)
        {
            max = std::max(grid[j][_index], max);
        }
        kvp.second.first = max;
        _index++;
    }
    
    return result;
}


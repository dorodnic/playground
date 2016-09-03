#include "ui.h"

#include "../easyloggingpp/easylogging++.h"

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

using namespace std;

#include "../stb/stb_easy_font.h"

inline std::string to_upper(std::string data)
{
    std::transform(data.begin(), data.end(), data.begin(), ::toupper);
    return data;
}

inline int get_text_width(const std::string& text)
{
    return stb_easy_font_width((char *)text.c_str());
}

inline void draw_text(int x, int y, const std::string& text)
{
    vector<char> buffer(2000 * text.size(), 0);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer.data());
    glDrawArrays(GL_QUADS, 0, 4*stb_easy_font_print((float)x, (float)y, 
                (char *)text.c_str(), nullptr, buffer.data(), buffer.size()));
    glDisableClientState(GL_VERTEX_ARRAY);
}

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
    auto x0 = _position.x.to_pixels(origin.size.x) + _margin.left;
    auto y0 = _position.y.to_pixels(origin.size.y) + _margin.top;

    auto size = get_size();
    auto w = min(size.x.to_pixels(origin.size.x),
                      origin.size.x - _margin.right - _margin.left);
    auto h = min(size.y.to_pixels(origin.size.y),
                      origin.size.y - _margin.bottom - _margin.top);

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

void Button::render(const Rect& origin)
{
    if (!is_visible()) return;

    glBegin(GL_QUADS);

    auto c = _color;
    if (is_enabled())
    {
        if (is_focused()) c = brighten(c, 1.1f);
        if (is_pressed(MouseButton::left)) c = brighten(c, 0.8f);
    }

    glColor3f(c.r, c.g, c.b);

    auto rect = arrange(origin);
    
    glVertex2i(rect.position.x, rect.position.y);
    glVertex2i(rect.position.x, rect.position.y + rect.size.y);
    glVertex2i(rect.position.x + rect.size.x,
               rect.position.y + rect.size.y);
    glVertex2i(rect.position.x + rect.size.x,
               rect.position.y);

    glEnd();
    
    _text_block.render(rect);
}

Size2 Button::get_intrinsic_size() const
{
    auto res = _text_block.get_intrinsic_size();
    return get_margin().apply(res);
}

Size2 TextBlock::get_intrinsic_size() const
{
    Size2 res = { get_text_width(to_upper(_text)) + 16, 20 };
    return get_margin().apply(res);
}

void TextBlock::render(const Rect& origin)
{
    if (!is_visible()) return;

    auto c = _color;
    glColor3f(c.r, c.g, c.b);
    
    auto rect = arrange(origin);
    auto text = to_upper(_text);
    
    auto text_width = get_text_width(text);
    auto text_height = 8;
    auto y_margin = rect.size.y / 2 - text_height / 2;
    auto text_y = rect.position.y + y_margin;

    if (get_alignment() == Alignment::left){
        draw_text(rect.position.x + y_margin, text_y, text);
    } else if (get_alignment() == Alignment::center){
        draw_text(rect.position.x + rect.size.x / 2 - text_width / 2, 
                  text_y, text);
    } else {
        draw_text(rect.position.x + rect.size.x - text_width - y_margin, 
                  text_y, text);
    }
}

void StackPanel::update_mouse_position(Int2 cursor)
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

    auto sum = get_arrangement().position.*ifield;
    set_focused_child(nullptr);
    for (auto& p : get_elements()) {
        auto new_origin = get_arrangement();
        (new_origin.position.*ifield) = sum;
        (new_origin.size.*ifield) = _sizes[p.get()].first;
        sum += _sizes[p.get()].first;

        //cout << new_origin << " - " << cursor << endl;

        if (contains(new_origin, cursor))
        {
            set_focused_child(p.get());
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

    auto panel = dynamic_cast<Container*>(item.get());
    if (panel)
    {
        panel->set_items_change([this](){
            invalidate_layout();
        });
    }

    _on_items_change();
    
    _focused = item.get();

    _content.push_back(item);
}

SizeMap StackPanel::calc_sizes(Orientation orientation,
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

Size2 StackPanel::get_intrinsic_size() const
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
}

void StackPanel::render(const Rect& origin)
{
    if (!is_visible()) return;

    Size Size2::* field;
    int Int2::* ifield;
    if (_orientation == Orientation::vertical) {
        field = &Size2::y;
        ifield = &Int2::y;
    } else {
        field = &Size2::x;
        ifield = &Int2::x;
    }

    if (update_layout(origin))
    {
        _sizes = calc_global_sizes(get_arrangement());
    }

    auto sum = get_arrangement().position.*ifield;
    for (auto& p : get_elements()) {
        auto new_origin = get_arrangement();
        (new_origin.position.*ifield) = sum;
        (new_origin.size.*ifield) = _sizes[p.get()].first;
        sum += _sizes[p.get()].first;
        p->render(new_origin);
    }
}

SimpleSizeMap Panel::calc_size_map(const Elements& content,
                                   const Rect& arrangement)
{
    SimpleSizeMap map;
    for (auto& p : content)
    {
        map[p.get()] = p->arrange(arrangement).size;
    }
    return map;
}

void Panel::render(const Rect& origin)
{
    if (!is_visible()) return;

    for (auto& p : get_elements()) {
        p->render(origin);
    }
}

Size2 Panel::get_intrinsic_size() const
{
    auto sizes = calc_size_map(get_elements(), { { 0, 0 }, { 0, 0 } });
    auto max_x = 0;
    auto max_y = 0;
    for (auto& kvp : sizes) {
        auto x = kvp.second.x;
        auto y = kvp.second.y;

        max_x = std::max(max_x, x);
        max_y = std::max(max_y, y);
    }

    return { Size(max_x), Size(max_y) };
}

void Grid::commit_line()
{
    if (_current_line) {
        StackPanel::add_item(_current_line);
        _lines.push_back(_current_line);
    }
    _current_line = shared_ptr<StackPanel>(
        new StackPanel("", {0,0}, { 1.0f, 1.0f }, 0, get_alignment(),
        get_orientation() == Orientation::vertical 
            ? Orientation::horizontal : Orientation::vertical,
        this));
}

SizeMap Grid::calc_sizes(const StackPanel* sender,
                         const Rect& arrangement) const
{
    map<const StackPanel*, SizeMap> maps;
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


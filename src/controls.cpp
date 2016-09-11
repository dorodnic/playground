#include "controls.h"

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

template<class T>
inline std::string stringify(T t)
{
    std::stringstream ss; ss << t;
    return ss.str();
}

inline int get_text_width(const std::string& text)
{
    return stb_easy_font_width((char *)text.c_str());
}

template<class T>
inline T lerp(T from, T to, float t)
{
    return to * t + from * (1.0f - t);
}

inline bool float_eq(float a, float b)
{
    return abs(a-b) < 0.001f;
}

inline int get_text_height() { return 8; }

inline void use(const Color3& c)
{
    glColor3f(c.r, c.g, c.b);
}

template<class T>
struct interval
{
    T from;
    T to;
    
    bool contains(const interval& other) const
    {
        return (other.from >= from) && (other.to <= to);
    }
    
    bool contains(T x) const 
    {
        return (x >= from) && (x <= to);
    }
    
    bool intersects(const interval& other) const
    {
        if (contains(other) || other.contains(*this)) return true;
        if (contains(other.from) || contains(other.to)) return true;
        return false;
    }
};

inline void draw_text(int x, int y, const std::string& text)
{
    vector<char> buffer(2000 * text.size(), 0);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer.data());
    glDrawArrays(GL_QUADS, 0, 4*stb_easy_font_print((float)x, (float)y, 
                (char *)text.c_str(), nullptr, buffer.data(), buffer.size()));
    glDisableClientState(GL_VERTEX_ARRAY);
}

void Button::render(const Rect& origin)
{
    glBegin(GL_QUADS);

    auto c = _color;
    if (is_enabled())
    {
        if (is_focused()) c = c.brighten(1.1f);
        if (is_pressed(MouseButton::left)) c = c.brighten(0.8f);
    }
    else
    {
        c = c.mix_with(c.to_grayscale(), 0.7);
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
    return _text_block.get_intrinsic_size();
}

Size2 TextBlock::get_intrinsic_size() const
{
    return { get_text_width(to_upper(_text)) + 16, 20 };
}

void TextBlock::render(const Rect& origin)
{
    auto c = _color;
    
    if (!is_enabled())
    {
        c = c.mix_with(c.to_grayscale(), 0.7);
    }
    
    glColor3f(c.r, c.g, c.b);
    
    auto rect = arrange(origin);
    auto text = to_upper(_text);
    
    auto text_width = get_text_width(text);
    auto text_height = get_text_height();
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


Size2 Slider::get_intrinsic_size() const
{
    return { 120, 20 };
}

void draw_diamond(float x, float y, float size)
{
    glVertex2i(x, y - size);
    glVertex2i(x + size, y);
    glVertex2i(x, y + size);
    glVertex2i(x - size, y);
}

void Slider::render(const Rect& origin) 
{
    auto bg_color = _color;
    auto txt_color = _text_color;
    if (!is_enabled())
    {
        bg_color = bg_color.mix_with(bg_color.to_grayscale(), 0.7);
        txt_color = txt_color.mix_with(txt_color.to_grayscale(), 0.7);
    }

    const auto pad = 1;
    auto rect = arrange(origin);

    auto x0 = rect.position.x;
    auto x1 = rect.position.x + rect.size.x;
    
    auto text_y = rect.position.y + rect.size.y - get_text_height();
    
    auto min_txt = stringify(_min);
    auto max_txt = stringify(_max);
    
    auto max_x = x1 - get_text_width(max_txt.c_str());
    auto min_x = x0 + get_text_width(min_txt.c_str());
    
    auto value_txt = stringify(_value);
    auto value_width = get_text_width(value_txt.c_str());
    auto v = (clamp(_value, _min, _max) - _min) / (_max - _min + 0.01f);
    auto value_x = (int)lerp(x0, x1, v);
    auto value_x0 = (int)(value_x - value_width / 2.0f);
    if (float_eq(_value, _min)) value_x0 = value_x;
    if (float_eq(_value, _max)) value_x0 = value_x - value_width;
    auto value_x1 = value_x0 + value_width;
    interval<int> value { value_x0, value_x1 };

    use(bg_color);
    
    
    glBegin(GL_QUADS);
    
    glVertex2i(x0 + pad, rect.position.y + pad);
    glVertex2i(x0 + pad, text_y - pad - 1);
    glVertex2i(x1 - pad,
               text_y - pad - 1);
    glVertex2i(x1 - pad,
               rect.position.y + pad);

    glEnd();
    
    use(txt_color);
    
    if (min_x <= value_x0) draw_text(x0, text_y, min_txt);
    if (max_x >= value_x1) draw_text(max_x, text_y, max_txt);
    draw_text(value_x0, text_y, value_txt);
    
    if (!float_eq(_step, 0.0f))
    {
        auto last_x = min_x;
        auto last_line_x = x0;
        for (auto i = _min; i < _max; i += _step)
        {
            auto marker_txt = stringify(i);
            auto marker_width = get_text_width(marker_txt);
            auto t = (i - _min) / (_max - _min + 0.01f);
            auto marker_center = lerp(x0, x1, t);
            auto marker_x0 = (int)(marker_center - marker_width / 2.0f);
            auto marker_x1 = marker_x0 + marker_width;
            
            interval<int> marker { marker_x0, marker_x1 };
            interval<int> free { last_x, max_x };
            
            if (marker_center > last_line_x + 8)
            {
                glBegin(GL_LINES);
                glVertex2i(marker_center, rect.position.y + pad + 1);
                glVertex2i(marker_center, text_y - pad - 2);
                glEnd();
                last_line_x = marker_center;
            }
            
            if (free.contains(marker) && !marker.intersects(value))
            {
                draw_text(marker_x0, text_y, marker_txt);
                last_x = marker_x1;
            }
        }
    }
    
    auto size = (text_y - rect.position.y) / 2 - pad;
    auto btn_y = rect.position.y + pad + size;
    auto btn_x = value_x;
    
    
    glBegin(GL_QUADS);
    
    draw_diamond(btn_x, btn_y, size);
    use(bg_color);
    draw_diamond(btn_x, btn_y, size - 3);

    glEnd();
}
    
    



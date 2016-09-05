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

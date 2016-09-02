#pragma once

#include <memory>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <map>

template<typename T>
inline T clamp(T val, T from, T to) { return std::max(from, std::min(to, val)); }

struct Color3 { float r, g, b; };
inline Color3 brighten(const Color3& c, float f)
{
    return { clamp(c.r * f, 0.0f, 1.0f),
             clamp(c.g * f, 0.0f, 1.0f),
             clamp(c.b * f, 0.0f, 1.0f) };
}
inline Color3 operator-(const Color3& c)
{
    return { 1.0f - c.r, 1.0f - c.g, 1.0f - c.b };
}

class Size
{
public:
    Size() : _pixels(0), _is_pixels(true) {} // Default is Auto
    Size(int pixels) : _pixels(pixels) {}
    Size(float percents)
        : _percents(percents), _is_pixels(false) {}

    static Size All() { return Size(1.0f); }
    static Size Nth(int n) { return Size(1.0f / n); }
    static Size Auto() { return Size(0); }

    int to_pixels(int base_pixels) const
    {
        if (_is_pixels) return clamp(_pixels, 0, base_pixels);
        else return (int) (_percents * base_pixels);
    }

    bool is_const() const { return _is_pixels; }
    float get_percents() const { return _percents; }
    int get_pixels() const { return _pixels; }
    bool is_auto() const { return _pixels == 0 && _is_pixels; }

private:
    int _pixels;
    float _percents;
    bool _is_pixels = true;
};

struct Size2 { Size x, y; };
inline Size2 Auto() { return { Size::Auto(), Size::Auto() }; }

struct Int2 { int x, y; };
inline bool operator==(const Int2& a, const Int2& b) {
    return (a.x == b.x) && (a.y == b.y);
}
inline std::ostream & operator << (std::ostream & o, const Int2& r) 
{ 
    return o << "(" << r.x << ", " << r.y << ")"; 
}

struct Rect { Int2 position, size; };
inline bool operator==(const Rect& a, const Rect& b)
{
    return (a.position == b.position) && (a.size == b.size);
}
inline std::ostream & operator << (std::ostream & o, const Rect& r) 
{ 
    return o << "[" << r.position << ", " << r.size << "]"; 
}

inline bool contains(const Rect& rect, const Int2& v)
{
    return (rect.position.x <= v.x && rect.position.x + rect.size.x >= v.x) &&
           (rect.position.y <= v.y && rect.position.y + rect.size.y >= v.y);
}

struct Margin
{
    int left, right, top, bottom;

    Margin(int v) : left(v), right(v), top(v), bottom(v) {}
    Margin(int x, int y) : left(x), right(x), top(y), bottom(y) {}
    Margin(int left, int top, int right, int bottom)
        : left(left), right(right), top(top), bottom(bottom) {}

    Margin operator-()
    {
        return { -left, -right, -top, -bottom };
    }

    Rect apply(const Rect& r) const
    {
        return { { r.position.x + left, r.position.y + top },
                 { r.size.x + left + right, r.size.y + top + bottom } };
    }
};

enum class MouseButton
{
    left,
    middle,
    right
};

enum class MouseState
{
    down,
    up
};

class IVisualElement
{
public:
    virtual Rect arrange(const Rect& origin) = 0;
    virtual void render(const Rect& origin) = 0;
    virtual const Margin& get_margin() const = 0;
    virtual Size2 get_size() const = 0;
    virtual Size2 get_intrinsic_size() const = 0;

    virtual void update_mouse_position(Int2 cursor) = 0;
    virtual void update_mouse_state(MouseButton button, MouseState state) = 0;
    virtual void update_mouse_scroll(Int2 scroll) = 0;

    virtual void focus(bool on) = 0;
    virtual bool is_focused() const = 0;

    virtual ~IVisualElement() {}
};

typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimePoint;

class MouseClickHandler : public virtual IVisualElement
{
public:
    MouseClickHandler();

    void update_mouse_state(MouseButton button, MouseState state) override;
    
    void update_mouse_scroll(Int2 scroll) override { // TODO
    };

    void set_on_click(std::function<void()> on_click,
                      MouseButton button = MouseButton::left)
    {
        _on_click[button] = on_click;
    }

    void set_on_double_click(std::function<void()> on_click){
        _on_double_click = on_click;
    }

    bool is_pressed(MouseButton button) const
    {
        auto it = _state.find(button);
        if (it != _state.end())
            return it->second == MouseState::down;
        else
            return false;
    }

private:
    std::map<MouseButton, MouseState> _state;
    std::map<MouseButton, TimePoint> _last_update;
    std::map<MouseButton, TimePoint> _last_click;

    std::map<MouseButton, std::function<void()>> _on_click;
    std::function<void()> _on_double_click;

    const int CLICK_TIME_MS = 200;
};

class ControlBase : public virtual IVisualElement,
                    public MouseClickHandler
{
public:
    ControlBase(const Size2& position,
                const Size2& size,
                const Margin& margin)
        : _position(position), _size(size), _margin(margin)
    {}

    Rect arrange(const Rect& origin) override;
    
    void update_mouse_position(Int2 cursor) override {}

    const Margin& get_margin() const override { return _margin; }
    Size2 get_size() const override;
    Size2 get_intrinsic_size() const override { return _size; };

    void focus(bool on) override { _focused = on; }
    bool is_focused() const override { return _focused; }

private:
    Size2 _position;
    Size2 _size;
    Margin _margin;
    bool _focused = false;
};

class Button : public ControlBase
{
public:
    Button(const Size2& position,
           const Size2& size,
           const Margin& margin,
           const Color3& color)
        : ControlBase(position, size, margin), _color(color)
    {}

    Size2 get_intrinsic_size() const override
    {
        return { 150, 35 }; // TODO: Once button will have text, change
                            // width to reflect text width
    };

    void render(const Rect& origin) override;

private:
    Color3 _color;
};

enum class Orientation
{
    vertical,
    horizontal
};

typedef std::unordered_map<IVisualElement*, std::pair<int, Size>> SizeMap;
typedef std::vector<std::shared_ptr<IVisualElement>> Elements;

class Container;

class ISizeCalculator
{
public:
    virtual SizeMap calc_sizes(const Container* sender,
                               const Rect& arrangement) const = 0;
};

class Container : public ControlBase
{
public:
    Container(const Size2& position,
              const Size2& size,
              const Margin& margin,
              Orientation orientation = Orientation::vertical,
              ISizeCalculator* resizer = nullptr)
        : ControlBase(position, size, margin),
          _orientation(orientation),
          _focused(nullptr),
          _on_items_change([](){}),
          _resizer(resizer)
    {}

    void update_mouse_position(Int2 cursor) override;

    void update_mouse_state(MouseButton button, MouseState state) override
    {
        if (_focused)
        {
            _focused->update_mouse_state(button, state);
        }
    }

    void invalidate_layout()
    {
        _origin = { { 0, 0 }, { 0, 0 } };
    }

    virtual void add_item(std::shared_ptr<IVisualElement> item);

    void set_items_change(std::function<void()> on_change)
    {
        _on_items_change = on_change;
    }

    static SizeMap calc_sizes(Orientation orientation,
                              const Elements& content,
                              const Rect& arrangement);

    SizeMap calc_local_sizes(const Rect& arrangement) const
    {
        return calc_sizes(_orientation,
                          _content,
                          arrangement);
    }

    SizeMap calc_global_sizes(const Rect& arrangement) const
    {
        if (_resizer) {
            return _resizer->calc_sizes(this, arrangement);
        } else {
            return calc_local_sizes(arrangement);
        }
    }

    Size2 get_intrinsic_size() const override;

    void render(const Rect& origin) override;
    
    Orientation get_orientation() const { return _orientation; }

private:
    Elements _content;
    Orientation _orientation;
    IVisualElement* _focused;

    Rect _origin;
    Rect _arrangement;
    SizeMap _sizes;

    std::function<void()> _on_items_change;
    
    ISizeCalculator* _resizer;
};

class Grid : public Container, public ISizeCalculator
{
public:
    Grid( const Size2& position,
          const Size2& size,
          const Margin& margin,
          Orientation orientation = Orientation::vertical)
        : Container(position, size, margin, orientation),
          _current_line(nullptr)
    {
        commit_line();
    }

    virtual void add_item(std::shared_ptr<IVisualElement> item)
    {
        _current_line->add_item(item);
    }

    void commit_line();


    SizeMap calc_sizes(const Container* sender,
                       const Rect& arrangement) const override;

private:
    std::shared_ptr<Container> _current_line;
    std::vector<std::shared_ptr<Container>> _lines;
};


#pragma once

#include <memory>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <map>

#include "../easyloggingpp/easylogging++.h"

enum class Orientation
{
    vertical,
    horizontal
};

template<typename T>
inline T clamp(T val, T from, T to) { return std::max(from, std::min(to, val)); }

inline float mix(float a, float b, float t)
{
    return a * (1 - t) + b * t;
}

struct Color3 { 
    float r, g, b; 
    
    Color3 brighten(float f) const
    {
        return { clamp(r * f, 0.0f, 1.0f),
                 clamp(g * f, 0.0f, 1.0f),
                 clamp(b * f, 0.0f, 1.0f) };
    }
    
    Color3 operator-() const
    {
        return { 1.0f - r, 1.0f - g, 1.0f - b };
    }
    
    Color3 to_grayscale() const
    {
        auto avg = (r + g + b) / 3;
        return { avg, avg, avg };
    }
    
    Color3 mix_with(const Color3& other, float t) const
    {
        return { mix(r, other.r, t),
                 mix(g, other.b, t),
                 mix(b, other.b, t) };
    }
};

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
    
    Size add_pixels(int px) const
    {
        if (_is_pixels && _pixels != 0) return Size(_pixels + px);
        else return *this;
    }

private:
    int _pixels;
    float _percents;
    bool _is_pixels = true;
};

struct Size2 
{ 
    Size x, y; 
};
inline std::ostream & operator << (std::ostream & o, const Size& r) 
{ 
    if (r.is_auto()) o << "auto";
    else if (r.is_const()) o << r.get_pixels() << "px";
    else o << (int)(r.get_percents() * 100) << "%";
}

inline Size2 Auto() { return { Size::Auto(), Size::Auto() }; }

inline std::ostream & operator << (std::ostream & o, const Size2& r) 
{ 
    return o << "{" << r.x << ", " << r.y << "}"; 
}

struct Int2 { 
    int x, y; 
};
inline bool operator==(const Int2& a, const Int2& b) {
    return (a.x == b.x) && (a.y == b.y);
}
inline std::ostream & operator << (std::ostream & o, const Int2& r) 
{ 
    return o << "(" << r.x << ", " << r.y << ")"; 
}
struct Rect 
{ 
    Int2 position, size; 
};
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
        
    operator bool() const { 
        return left != 0 || right != 0 || top != 0 || bottom != 0;
    }

    Margin operator-()
    {
        return { -left, -right, -top, -bottom };
    }

    Rect apply(const Rect& r) const
    {
        return { { r.position.x - left, r.position.y - top },
                 { r.size.x + left + right, r.size.y + top + bottom } };
    }
    
    Size2 apply(const Size2 s) const
    {
        return { s.x.add_pixels(left + right), s.y.add_pixels(top + bottom) };
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

enum class Alignment
{
    left,
    center,
    right
};

class IVisualElement
{
public:
    virtual Rect arrange(const Rect& origin) = 0;
    virtual void invalidate_layout() = 0;
    virtual void render(const Rect& origin) = 0;

    virtual Size2 get_size() const = 0;
    virtual Size2 get_intrinsic_size() const = 0;

    virtual void update_mouse_position(Int2 cursor) = 0;
    virtual void update_mouse_state(MouseButton button, MouseState state) = 0;
    virtual void update_mouse_scroll(Int2 scroll) = 0;

    virtual void focus(bool on) = 0;
    virtual bool is_focused() const = 0;
    virtual const std::string& get_name() const = 0;
    virtual std::string to_string() const = 0; 
    virtual const char* get_type() const = 0;
    virtual Alignment get_alignment() const = 0;
    
    virtual void set_enabled(bool on) = 0;
    virtual bool is_enabled() const = 0;
    
    virtual void set_visible(bool on) = 0;
    virtual bool is_visible() const = 0;
    
    virtual IVisualElement* find_element(const std::string& name) = 0;
    
    virtual void set_on_click(std::function<void()> on_click,
                              MouseButton button = MouseButton::left) = 0;
    virtual void set_on_double_click(std::function<void()> on_click) = 0;

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
                      MouseButton button = MouseButton::left) override
    {
        _on_click[button] = on_click;
    }

    void set_on_double_click(std::function<void()> on_click) override {
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
    ControlBase(std::string name,
                const Size2& position,
                const Size2& size,
                Alignment alignment)
        : _position(position), _size(size), 
          _name(name), _align(alignment), _parent(nullptr)
    {}

    Rect arrange(const Rect& origin) override;
    void invalidate_layout() override 
    {
        if (get_parent()) get_parent()->invalidate_layout();
        else LOG(INFO) << "UI Layout has invalidated! " << get_name();
    }
    
    void update_mouse_position(Int2 cursor) override {}

    Size2 get_size() const override;
    Size2 get_intrinsic_size() const override { return _size; };

    void focus(bool on) override { _focused = on; }
    bool is_focused() const override { return _focused; }
    
    const std::string& get_name() const override { return _name; }
    Alignment get_alignment() const override { return _align; }
    
    void set_enabled(bool on) override { _enabled = on; }
    bool is_enabled() const override { return _enabled; }
    void set_visible(bool on) override 
    { 
        _visible = on;
        invalidate_layout();
    }
    
    bool is_visible() const override { return _visible; }
    
    IVisualElement* find_element(const std::string& name) override
    {
        if (get_name() == name) return this;
        else return nullptr;
    }
    
    IVisualElement* get_parent() { return _parent; }
    const IVisualElement* get_parent() const { return _parent; }
    void update_parent(IVisualElement* new_parent) { _parent = new_parent; }
    
    std::string to_string() const override { return get_name() + "(" + get_type() + ")"; }

private:
    Size2 _position;
    Size2 _size;
    bool _focused = false;
    std::string _name;
    Alignment _align;
    bool _enabled = true;
    bool _visible = true;
    IVisualElement* _parent;
};


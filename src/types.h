#pragma once

#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <string>
#include <sstream>

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

typedef std::unordered_map<std::string, std::string> PropertyBag;

struct str
{
    operator std::string() const
    {
        return _ss.str();
    }
    
    template<class T>
    str& operator<<(T&& t) 
    { 
        _ss << t;
        return *this;
    }
    
    std::ostringstream _ss;
};

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
inline std::ostream & operator << (std::ostream & o, const Color3& c) 
{ 
    o << (int)(255 * c.r) << "," 
      << (int)(255 * c.g) << "," 
      << (int)(255 * c.b);
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
        if (_is_pixels) return _pixels;
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
    return o << r.x << ", " << r.y; 
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


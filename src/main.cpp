#include <iostream>
#include <memory>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <map>

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

using namespace std;

template<typename T>
T clamp(T val, T from, T to) { return std::max(from, std::min(to, val)); }

struct Color3 { float r, g, b; };
Color3 brighten(const Color3& c, float f)
{
    return { clamp(c.r * f, 0.0f, 1.0f),
             clamp(c.g * f, 0.0f, 1.0f),
             clamp(c.b * f, 0.0f, 1.0f) };
}
Color3 operator-(const Color3& c)
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
Size2 Auto() { return { Size::Auto(), Size::Auto() }; }

struct Int2 { int x, y; };
bool operator==(const Int2& a, const Int2& b) {
    return (a.x == b.x) && (a.y == b.y);
}
inline std::ostream & operator << (std::ostream & o, const Int2& r) { return o << "(" << r.x << ", " << r.y << ")"; }

struct Rect { Int2 position, size; };
bool operator==(const Rect& a, const Rect& b)
{
    return (a.position == b.position) && (a.size == b.size);
}
inline std::ostream & operator << (std::ostream & o, const Rect& r) { return o << "[" << r.position << ", " << r.size << "]"; }

bool contains(const Rect& rect, const Int2& v)
{
    return (rect.position.x <= v.x && rect.position.x + rect.size.x >= v.x) &&
           (rect.position.y <= v.y && rect.position.y + rect.size.y >= v.y);
}

struct Margin
{
    int left, right, top, bottom;

    Margin(int v) : left(v), right(v), top(v), bottom(v) {}
    Margin(int x, int y) : left(x), right(x), top(y), bottom(y) {}
    Margin(int left, int right, int top, int bottom)
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
    MouseClickHandler()
        : _on_double_click([this](){ _on_click[MouseButton::left](); })
    {
        _state[MouseButton::left] = _state[MouseButton::right] =
        _state[MouseButton::middle] = MouseState::up;

        _on_click[MouseButton::left] = _on_click[MouseButton::right] =
        _on_click[MouseButton::middle] = [](){};

        auto now = std::chrono::high_resolution_clock::now();
        _last_update[MouseButton::left] = _last_update[MouseButton::right] =
        _last_update[MouseButton::middle] = now;

        _last_click[MouseButton::left] = _last_click[MouseButton::right] =
        _last_click[MouseButton::middle] = now;
    }

    void update_mouse_state(MouseButton button, MouseState state) override
    {
        auto now = std::chrono::high_resolution_clock::now();
        auto curr = _state[button];
        if (curr != state)
        {
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>
                            (now - _last_update[button]).count();
            if (ms < CLICK_TIME_MS)
            {
                if (state == MouseState::up)
                {
                    ms = std::chrono::duration_cast<std::chrono::milliseconds>
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
    };

    void update_mouse_scroll(Int2 scroll) override {};

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

    Rect arrange(const Rect& origin) override
    {
        auto x0 = _position.x.to_pixels(origin.size.x) + _margin.left;
        auto y0 = _position.y.to_pixels(origin.size.y) + _margin.top;

        auto size = get_size();
        auto w = std::min(size.x.to_pixels(origin.size.x),
                          origin.size.x - _margin.right - _margin.left);
        auto h = std::min(size.y.to_pixels(origin.size.y),
                          origin.size.y - _margin.bottom - _margin.top);

        x0 += origin.position.x;
        y0 += origin.position.y;

        return { { x0, y0 }, { w, h } };
    }

    void update_mouse_position(Int2 cursor) override {}

    const Margin& get_margin() const override { return _margin; }
    Size2 get_size() const override
    {
        Size x = _size.x.is_auto() ? get_intrinsic_size().x : _size.x;
        Size y = _size.y.is_auto() ? get_intrinsic_size().y : _size.y;
        return { x, y };
    }
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

    void render(const Rect& origin) override
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

    void update_mouse_position(Int2 cursor) override
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

    virtual void add_item(std::shared_ptr<IVisualElement> item)
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

        _content.push_back(std::move(item));
    }

    void set_items_change(std::function<void()> on_change)
    {
        _on_items_change = on_change;
    }

    static SizeMap calc_sizes(Orientation orientation,
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
        std::vector<IVisualElement*> greedy;
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

        auto rest = std::max(arrangement.size.*ifield - sum, 0);
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

    Size2 get_intrinsic_size() const override
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

    void render(const Rect& origin) override
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

    void commit_line()
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


    SizeMap calc_sizes(const Container* sender,
                       const Rect& arrangement) const override
    {
        std::map<const Container*, SizeMap> maps;
        auto max_length = 0;
        for (auto& p : _lines)
        {
            auto map = p->calc_local_sizes(arrangement);
            maps[p.get()] = map;
            max_length = std::max(max_length, (int)map.size());
        }
        
        std::vector<std::vector<int>> grid;
        for (auto& p : _lines)
        {
            std::vector<int> row(max_length, 0);
            auto i = 0;
            for (auto& kvp : maps[p.get()])
            {
                row[i++] = kvp.second.first;
            }
            grid.push_back(row);
        }
        
        auto result = maps[sender];
        
        auto index = 0;
        for (auto& kvp : result)
        {
            auto max = 0;
            for (auto j = 0; j < grid.size(); j++)
            {
                max = std::max(grid[j][index], max);
            }
            kvp.second.first = max;
            index++;
        }
        
        return result;
    }

private:
    std::shared_ptr<Container> _current_line;
    std::vector<std::shared_ptr<Container>> _lines;
};

int main(int argc, char * argv[]) try
{
    glfwInit();
    GLFWwindow * win = glfwCreateWindow(1280, 960, "main", 0, 0);
    glfwMakeContextCurrent(win);

    Color3 redish { 0.8f, 0.5f, 0.6f };
    Color3 greenish { 0.6f, 0.9f, 0.6f };
    Color3 bluish { 0.5f, 0.5f, 0.8f };
    
    auto b = std::shared_ptr<Button>(new Button( { 0, 0 }, Auto(), { 5, 5, 5, 5 }, greenish ));
    auto b2 = std::shared_ptr<Button>(new Button( { 0, 0 }, { 1.0f, 1.0f }, { 5, 5, 5, 5 }, greenish ));

    
    Container c({ 0, 0 }, { 1.0f, 1.0f }, 0, Orientation::horizontal);
    
    auto left = std::shared_ptr<Container>(new Container( { 0, 0 }, { 1.0f, 1.0f }, 0, Orientation::vertical ));
    auto right = std::shared_ptr<Grid>(new Grid( { 0, 0 }, { 1.0f, 1.0f }, 0, Orientation::vertical ));

    for (int i = 0; i < 7; i++)
    {
        auto lrow = std::shared_ptr<Container>(new Container( { 0, 0 }, { 1.0f, 1.0f }, 0, Orientation::horizontal ));
         
        for (int j = 0; j < 7; j++)
        {
            auto w = rand() % 100;
            auto h = rand() % 100;
            
            lrow->add_item(std::shared_ptr<Button>(new Button( { 0, 0 }, { w, h }, 5, bluish )));
            right->add_item(std::shared_ptr<Button>(new Button( { 0, 0 }, { w, h }, 5, redish )));
        }
        
        
        left->add_item(lrow);
        right->commit_line();
    }
    
    c.add_item(left);
    c.add_item(right);
    

    glfwSetWindowUserPointer(win, &c);
    glfwSetCursorPosCallback(win, [](GLFWwindow * w, double x, double y) {
        auto ui_element = (IVisualElement*)glfwGetWindowUserPointer(w);
        ui_element->update_mouse_position({ (int)x, (int)y });
    });
    glfwSetScrollCallback(win, [](GLFWwindow * w, double x, double y) {
        auto ui_element = (IVisualElement*)glfwGetWindowUserPointer(w);
        ui_element->update_mouse_scroll({ (int)x, (int)y });
    });
    glfwSetMouseButtonCallback(win, [](GLFWwindow * w, int button, int action, int mods)
    {
        auto ui_element = (IVisualElement*)glfwGetWindowUserPointer(w);
        MouseButton button_type;
        switch(button)
        {
        case GLFW_MOUSE_BUTTON_RIGHT:
            button_type = MouseButton::right; break;
        case GLFW_MOUSE_BUTTON_LEFT:
            button_type = MouseButton::left; break;
        case GLFW_MOUSE_BUTTON_MIDDLE:
            button_type = MouseButton::middle; break;
        default:
            button_type = MouseButton::left;
        };

        MouseState mouse_state;
        switch(action)
        {
        case GLFW_PRESS:
            mouse_state = MouseState::down;
            break;
        case GLFW_RELEASE:
            mouse_state = MouseState::up;
            break;
        default:
            mouse_state = MouseState::up;
        };

        ui_element->update_mouse_state(button_type, mouse_state);
    });

    while (!glfwWindowShouldClose(win))
    {
        glfwPollEvents();

        int w,h;
        glfwGetFramebufferSize(win, &w, &h);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);

        glPushMatrix();
        glfwGetWindowSize(win, &w, &h);
        glOrtho(0, w, h, 0, -1, +1);

        Rect origin { { 0, 0 }, { w, h } };

        c.render(origin);

        glPopMatrix();
        glfwSwapBuffers(win);
    }

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
catch(const std::exception & e)
{
    std::cerr << e.what() << std::endl;
    return -1;
}

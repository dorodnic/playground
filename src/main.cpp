#include <iostream>
#include <memory>
#include <vector>
#include <unordered_map>

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

using namespace std;

template<typename T>
T clamp(T val, T from, T to) { return std::max(from, std::min(to, val)); }

struct Color3 { float r, g, b; };

class Size
{
public:
    Size(int pixels) : _pixels(pixels) {}
    Size(float percents) 
        : _percents(percents), _is_pixels(false) {}
    
    static Size All() { return Size(1.0f); }
    static Size Nth(int n) { return Size(1.0f / n); }
    
    int to_pixels(int base_pixels) const 
    { 
        if (_is_pixels) return clamp(_pixels, 0, base_pixels); 
        else return (int) (_percents * base_pixels);
    }
    
    bool is_const() const { return _is_pixels; }
    float get_percents() const { return _percents; }
    int get_pixels() const { return _pixels; }
    
private:
    int _pixels;
    float _percents;
    bool _is_pixels = true;
};

struct Size2 { Size x, y; };
struct Int2 { int x, y; };
struct Rect { Int2 position, size; };
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

class IVisualElement
{
public:
    virtual Rect arrange(const Rect& origin) = 0;
    virtual void render(const Rect& origin) = 0;
    virtual const Margin& get_margin() const = 0;
    virtual const Size2& get_size() const = 0;
    
    virtual ~IVisualElement() {}
};

class ControlBase : public IVisualElement
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
        
        auto w = std::min(_size.x.to_pixels(origin.size.x),
                          origin.size.x - _margin.right - _margin.left);
        auto h = std::min(_size.y.to_pixels(origin.size.y),
                          origin.size.y - _margin.bottom - _margin.top);
        
        x0 += origin.position.x;
        y0 += origin.position.y;
        
        return { { x0, y0 }, { w, h } };
    }
    
    const Margin& get_margin() const override { return _margin; }
    const Size2& get_size() const override { return _size; }
    
private:
    Size2 _position;
    Size2 _size;
    Margin _margin;
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

    void render(const Rect& origin) override
    {
        glBegin(GL_QUADS);
        glColor3f(_color.r, _color.g, _color.b);
        
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

class Container : public ControlBase
{
public:
    Container(const Size2& position, 
              const Size2& size, 
              const Margin& margin,
              Orientation orientation = Orientation::vertical)
        : ControlBase(position, size, margin), _orientation(orientation)
    {}
    
    void add_item(std::unique_ptr<IVisualElement> item)
    {
        _content.push_back(std::move(item));
    }
    
    void render(const Rect& origin) override
    {
        auto rect = arrange(origin);
    
        Size Size2::* field;
        int Int2::* ifield;
        if (_orientation == Orientation::vertical) {
            field = &Size2::y;
            ifield = &Int2::y;
        } else {
            field = &Size2::x;
            ifield = &Int2::x;
        }
    
        // first, scan items, map the "greedy" ones wanting relative portion
        std::vector<IVisualElement*> greedy;
        std::unordered_map<IVisualElement*, int> sizes;
        auto sum = 0;
        for (auto& p : _content) {
            auto p_rect = p->arrange(rect);
            auto p_total = p->get_margin().apply(p_rect);
            
            if ((p->get_size().*field).is_const()) {
                auto pixels = (p->get_size().*field).get_pixels();
                sum += pixels;
                sizes[p.get()] = pixels;
            } else {
                greedy.push_back(p.get());
            }
        }
        
        auto rest = std::max(rect.size.*ifield - sum, 0);
        float total_parts = 0;
        for (auto ptr : greedy) {
            total_parts += (ptr->get_size().*field).get_percents();
        }
        for (auto ptr : greedy) {
            auto f = ((ptr->get_size().*field).get_percents() / total_parts);
            sizes[ptr] = (int) (rest * f);
        }
        
        sum = rect.position.*ifield;
        for (auto& p : _content) {
            auto new_origin = rect;
            (new_origin.position.*ifield) = sum;
            (new_origin.size.*ifield) = sizes[p.get()];
            sum += sizes[p.get()];
            p->render(new_origin);
        }
    }
    
private:
    std::vector<std::unique_ptr<IVisualElement>> _content;
    Orientation _orientation;
};

int main(int argc, char * argv[]) try
{
    glfwInit();
    GLFWwindow * win = glfwCreateWindow(1280, 960, "main", 0, 0);
    glfwMakeContextCurrent(win);

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
        
        Color3 redish { 0.8f, 0.5f, 0.6f };
        
        Container c( { 0, 0 }, { 300, 1.0f }, { 5, 5, 5, 5 } );
        c.add_item(std::unique_ptr<Button>(new Button( { 0, 0 }, { 1.0f, 35 }, { 5, 5, 5, 5 }, redish )));
        c.add_item(std::unique_ptr<Button>(new Button( { 0, 0 }, { 1.0f, 1.0f }, { 5, 5, 5, 5 }, redish )));
        c.add_item(std::unique_ptr<Button>(new Button( { 0, 0 }, { 1.0f, 35 }, { 5, 5, 5, 5 }, redish )));
        c.add_item(std::unique_ptr<Button>(new Button( { 0, 0 }, { 1.0f, 35 }, { 5, 5, 5, 5 }, redish )));
        
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

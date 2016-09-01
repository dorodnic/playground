#include <iostream>
#include <memory>

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
};

class Button
{
public:
    Button(const Size2& position, 
           const Size2& size, 
           const Margin& margin,
           const Color3& color)
        : _position(position), _size(size), _color(color), _margin(margin)
    {}

    void render(const Rect& origin)
    {
        glBegin(GL_QUADS);
        glColor3f(_color.r, _color.g, _color.b);
        
        auto x0 = _position.x.to_pixels(origin.size.x);
        auto y0 = _position.y.to_pixels(origin.size.y);
        
        auto w = _size.x.to_pixels(origin.size.x);
        auto h = _size.y.to_pixels(origin.size.y);
        
        x0 += origin.position.x;
        y0 += origin.position.y;
        
        glVertex2i(x0, y0);
        glVertex2i(x0, y0 + h);
        glVertex2i(x0 + w, y0 + h);
        glVertex2i(x0 + w, y0);
        glEnd();
    }
    
private:
    Size2 _position;
    Size2 _size;
    Color3 _color;
    Margin _margin;
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
        
        Button b( { 50, 50 }, { 0.5f, 35 }, { 0, 0, 0, 0 }, redish );
        b.render(origin);

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

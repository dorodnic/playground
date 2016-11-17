#pragma once

#include "shader.h"
#include "types.h"

#include <string>
#include <unordered_map>
#include <vector>

class Flat2dRect
{
public:
    Flat2dRect(const Rect& rect,
               const Color3& color);
    
    const Color3& get_color() const { return _color; }
    
    unsigned int get_vao() const { return _vao; }
    
    ~Flat2dRect();
private:
    const Color3 _color;
    unsigned int _vao;
    unsigned int _vbo;
};

class Flat2dRenderer
{
public:
    Flat2dRenderer();
    
    void set_window_size(const Int2& size);
    
    void render(const Flat2dRect& rect) const;
    
private:
    std::unique_ptr<ShaderProgram> _shader;
    Int2 _size;
};

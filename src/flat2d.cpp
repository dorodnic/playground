#include "flat2d.h"

#include "../easyloggingpp/easylogging++.h"

#ifdef WIN32
    #define USEGLEW
    #include <GL/glew.h>
#endif

#define GLFW_INCLUDE_GLU
#define GLFW_INCLUDE_GLEXT
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>

Flat2dRenderer::Flat2dRenderer()
{
    _shader = ShaderProgram::load("resources/shaders/flat2d_vertex.c",
                                  "resources/shaders/flat2d_fragment.c");
}

void Flat2dRenderer::set_window_size(const Int2& size)
{
    _size = size;
}

void Flat2dRenderer::render(const Flat2dRect& rect) const
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    _shader->begin();
 
    auto c = rect._color;
	GLint myLoc = glGetUniformLocation(_shader->get_id(), "rect_color");
	glUniform3f(myLoc, c.r, c.g, c.b);
    
    myLoc = glGetUniformLocation(_shader->get_id(), "screen_size");
    glUniform2f(myLoc, _size.x, _size.y);
    

    myLoc = glGetUniformLocation(_shader->get_id(), "rect_size");
    glUniform2f(myLoc, rect._rect.size.x, rect._rect.size.y);


    myLoc = glGetUniformLocation(_shader->get_id(), "rounding_size");
    glUniform1f(myLoc, rect._rounding);

    glBindBuffer(GL_ARRAY_BUFFER, rect._vbo);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, rect._bvbo);
    glEnableVertexAttribArray(1);

    glBindVertexArray(rect._vao);
    glDrawArrays(GL_QUADS, 0, 4);
    
    _shader->end();

    glDisable(GL_BLEND);
}

Flat2dRect::Flat2dRect(const Rect& rect,
                       const Color3& color)
    : _color(color), _rect(rect)
{
    auto x0 = rect.position.x;
    auto y0 = rect.position.y;
    auto x1 = rect.position.x + rect.size.x;
    auto y1 = rect.position.y + rect.size.y;
    
    std::vector<float> vertex_positions;
    vertex_positions.reserve(8);
    vertex_positions.push_back(x0);
    vertex_positions.push_back(y0);
    vertex_positions.push_back(x1);
    vertex_positions.push_back(y0);
    vertex_positions.push_back(x1);
    vertex_positions.push_back(y1);
    vertex_positions.push_back(x0);
    vertex_positions.push_back(y1);

    float barycentric_coords[] { 0, 0, rect.size.x, 0, rect.size.x,
                                 rect.size.y, 0, rect.size.y };

    
    GLuint vao, vbo, bvbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertex_positions.size() * sizeof(float),
                 vertex_positions.data(), GL_STATIC_DRAW);
                 
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);


    glGenBuffers(1, &bvbo);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, bvbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(barycentric_coords),
                 barycentric_coords, GL_STATIC_DRAW);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    _vao = vao;
    _vbo = vbo;
    _bvbo = bvbo;
}

Flat2dRect::~Flat2dRect()
{
    glDeleteBuffers(1, &_vbo);
    glDeleteBuffers(1, &_bvbo);
    glDeleteBuffers(1, &_vao);
}

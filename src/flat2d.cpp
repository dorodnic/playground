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

void Flat2dRenderer::set_window_size(const Int2& size) const
{
    _shader->begin();
    
	GLint myLoc = glGetUniformLocation(_shader->get_id(), "screen_size");
	glUniform2f(myLoc, size.x, size.y);
    
    _shader->end();
}

void Flat2dRenderer::render(const Flat2dRect& rect) const
{
    _shader->begin();
 
    auto c = rect.get_color();
	GLint myLoc = glGetUniformLocation(_shader->get_id(), "rect_color");
	glUniform3f(myLoc, c.r, c.g, c.b);
    
    myLoc = glGetUniformLocation(_shader->get_id(), "screen_size");
	glUniform2f(myLoc, 640, 480);
    
    glBindVertexArray(rect.get_vao());
    glDrawArrays(GL_QUADS, 0, 4);
    
    _shader->end();
}

Flat2dRect::Flat2dRect(const Rect& rect,
                       const Color3& color)
    : _color(color)
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
    
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertex_positions.size() * sizeof(float), 
                 vertex_positions.data(), GL_STATIC_DRAW);
                 
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    _vao = vao;
    _vbo = vbo;
}

Flat2dRect::~Flat2dRect()
{
    glDeleteBuffers(1, &_vbo);
    glDeleteBuffers(1, &_vao);
}
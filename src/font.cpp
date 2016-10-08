#include "font.h"

#include <chrono>

#include "types.h"

#include "parser.h"

#include "../easyloggingpp/easylogging++.h"

#ifdef WIN32
    #define USEGLEW
    #include <GL/glew.h>
#endif

#define GLFW_INCLUDE_GLU
#define GLFW_INCLUDE_GLEXT
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../stb/stb_image.h"

using namespace std;

int get_param(const std::string& param, MinimalParser& line, int line_number)
{
    line.get_spacing();
    auto term = line.get_id();
    if (term != param) 
        throw std::runtime_error(str() << "Error parsing font file!"
        << " At line " << line_number << " identifier " << param << " expected,"
        << "got " << term << "!");
    line.req('=');
    return line.get_int();
}

std::string get_string_param(const std::string& param, MinimalParser& line, int line_number)
{
    line.get_spacing();
    auto term = line.get_id();
    if (term != param) 
        throw std::runtime_error(str() << "Error parsing font file!"
        << " At line " << line_number << " identifier " << param << " expected,"
        << "got " << term << "!");
    line.req('=');
    return line.get_string_literal();
}

Margin get_margin_param(const std::string& param, MinimalParser& line, int line_number)
{
    line.get_spacing();
    auto term = line.get_id();
    if (term != param) 
        throw std::runtime_error(str() << "Error parsing font file!"
        << " At line " << line_number << " identifier " << param << " expected,"
        << "got " << term << "!");
    line.req('=');
    return line.get_margin();
}

int FontLoader::get_kerning(char a, char b) const
{
    char buff[3];
    buff[0] = a;
    buff[1] = b;
    buff[2] = '\0';
    std::string s(buff);
    auto it = _kerning.find(s);
    if (it != _kerning.end())
    {
        return it->second;
    }
    else
    {
        return 0;
    }
}

TextMesh::TextMesh(const FontLoader& font, const std::string& text, 
                   int size, float sdf_width, float sdf_edge,
                   const Int2& position, const Color3& color)
    : _color(color), _font(font), _position(position)
{   
    auto x = 0;
    auto y = 0;
    
    _size_ratio = size / (float)font.get_native_size();
    _sdf_width = sdf_width;
    _sdf_edge = sdf_edge;
    
    auto tex_scale = 1.0f / font.get_texture_size();
    
    _vertex_positions.reserve(text.size() * 8);
    _texture_coords.reserve(text.size() * 8);
    
    auto min_y = 1000;
    auto max_y = -1000;
    
    for (auto i = 0; i < text.size(); i++)
    {
        auto c = text[i];
        auto& fc = *font.lookup(c);
        auto x0 = x + fc.xoffset;
        auto y0 = y - fc.yoffset;
        auto x1 = x0 + fc.width;
        auto y1 = y0 - fc.height;
        
        min_y = std::min(min_y, y0);
        min_y = std::min(min_y, y1);
        max_y = std::max(max_y, y0);
        max_y = std::max(max_y, y1);
        
        _vertex_positions.push_back(x0);
        _vertex_positions.push_back(y0);
        _vertex_positions.push_back(x1);
        _vertex_positions.push_back(y0);
        _vertex_positions.push_back(x1);
        _vertex_positions.push_back(y1);
        _vertex_positions.push_back(x0);
        _vertex_positions.push_back(y1);
        
        _texture_coords.push_back(tex_scale * fc.x);
        _texture_coords.push_back(tex_scale * fc.y);
        
        _texture_coords.push_back(tex_scale * (fc.x + fc.width));
        _texture_coords.push_back(tex_scale * fc.y);
        
        _texture_coords.push_back(tex_scale * (fc.x + fc.width));
        _texture_coords.push_back(tex_scale * (fc.y + fc.height));
        
        _texture_coords.push_back(tex_scale * fc.x);
        _texture_coords.push_back(tex_scale * (fc.y + fc.height));
        

        x += fc.xadvance - font.get_advance_adjustment();
        
        if (i+1 < text.size())
        {
            x += font.get_kerning(text[i], text[i+1]);
        }
    }
    
    _width = x;
    _height = max_y - min_y;
    
    GLuint vao, vbo, vbo2;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, get_size(), get_vertex_positions(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    
	glGenBuffers(1, &vbo2);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, vbo2);
	
	glBufferData(GL_ARRAY_BUFFER, get_size(), get_texture_coords(), GL_STATIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    _vao = vao;
    _vertex_vbo = vbo;
    _uv_vbo = vbo2;
}

TextMesh::~TextMesh()
{
    glDeleteBuffers(1, &_vertex_vbo);
	glDeleteBuffers(1, &_uv_vbo);
    glDeleteBuffers(1, &_vao);
}

FontLoader::FontLoader(const std::string& filename)
{
    LOG(INFO) << "Loading font " << filename << "...";
    auto started = chrono::high_resolution_clock::now();
    
    ifstream theFile(str() << "resources/fonts/" << filename);
    auto buffer = vector<char>((istreambuf_iterator<char>(theFile)), 
                                 istreambuf_iterator<char>());
    buffer.push_back('\0');
    
    std::string texture_filename;
    
    MinimalParser parser(buffer.data());
    int line_number = 1;
    while (!parser.eof())
    {
        auto ls = parser.get_line();
        MinimalParser line(ls);
        if (line.eof()) continue;
        
        auto id = line.get_id();
        if (id == "char")
        {
            auto id = get_param("id", line, line_number);
            auto x = get_param("x", line, line_number);
            auto y = get_param("y", line, line_number);
            auto w = get_param("width", line, line_number);
            auto h = get_param("height", line, line_number);
            auto xoffset = get_param("xoffset", line, line_number);
            auto yoffset = get_param("yoffset", line, line_number);
            auto xadvance = get_param("xadvance", line, line_number);
            line.rest();
            
            FontCharacter c { (char)id, x, y, w, h, xoffset, yoffset, xadvance };
            _chars[(char)id] = c;
        }
        else if (id == "kerning")
        {
            //kerning first=193 second=84 amount=-5
            
            auto first = get_param("first", line, line_number);
            auto second = get_param("second", line, line_number);
            auto amount = get_param("amount", line, line_number);
            line.req_eof();
            
            std::string combined = str() << (char)first << (char)second;
            _kerning[combined] = amount;
        }
        else if (id == "common")
        {
            //common lineHeight=99 base=56 scaleW=1024 scaleH=1024 pages=1 packed=0
            
            get_param("lineHeight", line, line_number);
            get_param("base", line, line_number);
            _texture_size = get_param("scaleW", line, line_number);
            line.rest();
        }
        else if (id == "info")
        {
            //info face="Bitstream Vera Sans" size=60 bold=0 italic=0 charset="" unicode=0 stretchH=100 smooth=1 aa=1 padding=15,15,15,15 spacing=-2,-2
            
            get_string_param("face", line, line_number);
            _size = get_param("size", line, line_number);
            get_param("bold", line, line_number);
            get_param("italic", line, line_number);
            get_string_param("charset", line, line_number);
            get_param("unicode", line, line_number);
            get_param("stretchH", line, line_number);
            get_param("smooth", line, line_number);
            get_param("aa", line, line_number);
            
            auto margin = get_margin_param("padding", line, line_number);
            auto spacing = get_margin_param("spacing", line, line_number);
            _advance_adjustment = margin.left + margin.right 
                                + spacing.left + spacing.right;
        }
        else if (id == "page")
        {
            //page id=0 file="v.png"
            get_param("id", line, line_number);
            texture_filename = get_string_param("file", line, line_number);
        }
        else
        {
            line.rest();
        }
        
        line_number++;
    }
    
    std::string name = str() << "resources/fonts/" << texture_filename;
    const char * cstr = name.c_str();

    //stb_image
    int x, y, comp;
    FILE *fh = fopen(cstr, "rb");
    unsigned char *res;
    res = stbi_load_from_file(fh,&x,&y,&comp,0);
    fclose(fh);
    
    // Create one OpenGL texture
    GLuint textureID;
    glGenTextures(1, &textureID);
    _texture_id = textureID;

    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Give the image to OpenGL
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, res);
    
    stbi_image_free(res);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    auto ended = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(ended - started).count();
    LOG(INFO) << filename << " loaded, took " << duration << "ms";
    
}

FontLoader::~FontLoader()
{
    glDeleteTextures(1, &_texture_id);
}

void FontRenderer::set_window_size(const Int2& size) const
{
    _shader->begin();
    
	GLint myLoc = glGetUniformLocation(_shader->get_id(), "screen_size");
	glUniform2f(myLoc, size.x, size.y);
    
    _shader->end();
}

FontRenderer::FontRenderer()
{
    _shader = ShaderProgram::load("resources/shaders/font_vertex.c",
                                  "resources/shaders/font_fragment.c");
}

void FontLoader::begin() const
{
    glBindTexture(GL_TEXTURE_2D, _texture_id);
    glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void FontLoader::end() const 
{
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void FontRenderer::render(const TextMesh& mesh) const
{
    _shader->begin();
    mesh.get_font().begin();

    auto c = mesh.get_color();
	GLint myLoc = glGetUniformLocation(_shader->get_id(), "font_color");
	glUniform3f(myLoc, c.r, c.g, c.b);
    
    auto position = mesh.get_position();
    myLoc = glGetUniformLocation(_shader->get_id(), "text_position");
	glUniform2f(myLoc, position.x, position.y);
    
    auto size_ratio = mesh.get_size_ratio();
    myLoc = glGetUniformLocation(_shader->get_id(), "size_ratio");
	glUniform1f(myLoc, size_ratio);
    
    auto sdf_width = mesh.get_sdf_width();
    myLoc = glGetUniformLocation(_shader->get_id(), "sdf_width");
	glUniform1f(myLoc, sdf_width);
    
    auto sdf_edge = mesh.get_sdf_edge();
    myLoc = glGetUniformLocation(_shader->get_id(), "sdf_edge");
	glUniform1f(myLoc, sdf_edge);
    
    glBindVertexArray(mesh.get_vao());
    glDrawArrays(GL_QUADS, 0, mesh.get_vertex_count());
    
    mesh.get_font().end();
    _shader->end();
}
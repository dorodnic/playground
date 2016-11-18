#pragma once

#include "shader.h"
#include "types.h"
#include "bind.h"

#include <string>
#include <unordered_map>
#include <vector>

class FontLoader;

class TextMesh
{
public:
    int get_vertex_count() const { return _vertex_positions.size() / 2; }
    
    int get_width() const { return _width * _size_ratio; }
    int get_height() const { return _height * _size_ratio; }
    
    int get_size() const { return _vertex_positions.size() * sizeof(float); }
    
    float* get_vertex_positions() { return _vertex_positions.data(); }
    float* get_texture_coords() { return _texture_coords.data(); }

    TextMesh(const TextMesh&) = delete;
    
    TextMesh(const FontLoader& font, 
             const std::string& text,
             int size, float sdf_width, float sdf_edge,
             const Int2& position,
             const Color3& c);
    
    unsigned int get_vao() const { return _vao; }
    unsigned int get_vbo() const { return _vertex_vbo; }
    unsigned int get_uv_vbo() const { return _uv_vbo; }
    
    const Color3& get_color() const { return _color; }
    const Int2& get_position() const { return _position; }
    void set_position(const Int2& pos) { _position = pos; }
    
    const FontLoader& get_font() const { return _font; }
    
    float get_size_ratio() const { return _size_ratio; }
    float get_sdf_width() const { return _sdf_width; }
    float get_sdf_edge() const { return _sdf_edge; }

    void set_text_size(float size);

    ~TextMesh();

private:
    std::vector<float> _vertex_positions;
    std::vector<float> _texture_coords;
    int _width;
    int _height;
    unsigned int _vao;
    unsigned int _vertex_vbo;
    unsigned int _uv_vbo;
    
    float _size_ratio;
    
    float _sdf_width;
    float _sdf_edge;
    
    Color3 _color;
    const FontLoader& _font;
    Int2 _position;
};

struct FontCharacter
{
    char id;
    int x;
    int y;
    int width;
    int height;
    int xoffset;
    int yoffset;
    int xadvance;
};

class FontRenderer
{
public:
    FontRenderer();
    
    void render(const TextMesh& mesh) const;
    
    void set_window_size(const Int2& size) const;
    
private:
    std::unique_ptr<ShaderProgram> _shader;
};

class FontLoader
{
public:
    explicit FontLoader(const std::string& filename);
    
    const FontCharacter* lookup(char c) const {
        auto it = _chars.find(c);
        if (it != _chars.end()) return &it->second;
        else return nullptr;
    }
	
	int get_kerning(char a, char b) const;
	
	int get_texture_size() const { return _texture_size; }
    
    int get_native_size() const { return _size; }
	
	int get_advance_adjustment() const { return _advance_adjustment; }
	
    void begin() const;
    void end() const;

	~FontLoader();
	
private:
    std::unordered_map<char, FontCharacter> _chars;
    std::unordered_map<std::string, int> _kerning;
    
	int _texture_size;
    int _size;
	int _advance_adjustment;
    unsigned int _texture_id;
};

class Font : public BindableObjectBase
{
public:
    Font() : _src(""), _loader(nullptr) {}
    
    const std::string& get_src() const
    {
        return _src;
    }
    
    void set_src(const std::string& src)
    {
        if (src != _src)
        {
            _loader.reset(new FontLoader(src));
            _src = src;
            fire_property_change("src");
        }
    }
    
    const FontLoader& get_loader() const
    {
        return *_loader;
    }
    
private:
    std::unique_ptr<FontLoader> _loader;
    std::string _src;
};

template<>
struct TypeDefinition<Font>
{
    static std::shared_ptr<ITypeDefinition> make() 
    {
        DefineClass(Font)
             ->AddProperty(get_src, set_src);
    }
};

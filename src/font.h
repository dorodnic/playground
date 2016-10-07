#pragma once

#include <string>
#include <unordered_map>
#include <vector>

class Font;

class TextMesh
{
public:
    int get_vertex_count() const { return _vertex_positions.size() / 2; }
    
    int get_width() const { return _width; }
    int get_height() const { return _height; }
    
    int get_size() const { return _vertex_positions.size() * sizeof(float); }
    
    float* get_vertex_positions() { return _vertex_positions.data(); }
    float* get_texture_coords() { return _texture_coords.data(); }
    
    void get_vertex(int i, float& x, float& y) { 
        x = _vertex_positions[i * 2];
        y = _vertex_positions[i * 2 + 1];
    }
    
    TextMesh(const Font& font, const std::string& text);

private:
    std::vector<float> _vertex_positions;
    std::vector<float> _texture_coords;
    int _width;
    int _height;
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

class Font
{
public:
    explicit Font(const std::string& filename);
    
    const FontCharacter* lookup(char c) const {
        auto it = _chars.find(c);
        if (it != _chars.end()) return &it->second;
        else return nullptr;
    }
	
	int get_kerning(char a, char b) const;
	
	int get_size() const { return _size; }
    
private:
    std::unordered_map<char, FontCharacter> _chars;
    std::unordered_map<std::string, int> _kerning;
    int _texture_id;
	int _size;
};

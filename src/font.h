#pragma once

#include <string>
#include <unordered_map>



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
    static Font load(const std::string& filename);
    
    
    
private:
    std::vector<FontCharacter> _chars;
    std::unordered_map<std::string, int> _kerning;
    int _texture_id;
};

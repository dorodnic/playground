#include "font.h"

#include <chrono>

#include "types.h"

#include "parser.h"

#include "../easyloggingpp/easylogging++.h"

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

TextMesh::TextMesh(const Font& font, const std::string& text)
{
    auto x = 0;
    
    _vertex_positions.resize(text.size() * 4);
    _texture_coords.resize(text.size() * 4);
    
    auto min_y = 1000;
    auto max_y = -1000;
    
    for (auto i = 0; i < text.size(); i++)
    {
        auto c = text[i];
        auto& fc = *font.lookup(c);
        auto x0 = x + fc.xoffset;
        auto y0 = fc.yoffset;
        auto x1 = x0 + fc.width;
        auto y1 = y0 + fc.height;
        
        min_y = std::min(min_y, y0);
        min_y = std::min(min_y, y1);
        max_y = std::max(max_y, y0);
        max_y = std::max(max_y, y1);
        
        _vertex_positions.push_back(x0);
        _vertex_positions.push_back(y0);
        _vertex_positions.push_back(x1);
        _vertex_positions.push_back(y1);
        
        _texture_coords.push_back(fc.x);
        _texture_coords.push_back(fc.y);
        _texture_coords.push_back(fc.x + fc.width);
        _texture_coords.push_back(fc.y + fc.height);
        
        x += fc.xadvance;
    }
    
    _width = x;
    _height = max_y - min_y;
}

Font::Font(const std::string& filename)
{
    LOG(INFO) << "Loading font " << filename << "...";
    auto started = chrono::high_resolution_clock::now();
    
    ifstream theFile(filename);
    auto buffer = vector<char>((istreambuf_iterator<char>(theFile)), 
                                 istreambuf_iterator<char>());
    buffer.push_back('\0');
    
    MinimalParser parser(buffer.data());
    int line_number = 1;
    while (!parser.eof())
    {
        auto ls = parser.get_line();
        MinimalParser line(ls);
        if (line.eof()) continue;
        
        auto id = line.get_id();
        LOG(INFO) << ls;
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
        else
        {
            line.rest();
        }
        
        line_number++;
    }
    
    auto ended = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(ended - started).count();
    LOG(INFO) << filename << " loaded, took " << duration << "ms";
}

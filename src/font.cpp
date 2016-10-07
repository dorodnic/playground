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

int Font::get_kerning(char a, char b) const
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

TextMesh::TextMesh(const Font& font, const std::string& text)
{
	/*_vertex_positions.push_back(0);
	_vertex_positions.push_back(0);
	_vertex_positions.push_back(1);
	_vertex_positions.push_back(0);
	_vertex_positions.push_back(1);
	_vertex_positions.push_back(1);
	_vertex_positions.push_back(0);
	_vertex_positions.push_back(1);
	
	_texture_coords.push_back(0);
	_texture_coords.push_back(0);
	_texture_coords.push_back(1);
	_texture_coords.push_back(0);
	_texture_coords.push_back(1);
	_texture_coords.push_back(1);
	_texture_coords.push_back(0);
	_texture_coords.push_back(1);*/
	
    auto x = 200;
    auto y = 200;
	auto scale = 0.001f;
	
	auto tex_scale = 1.0f / font.get_size();
    
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
        
        _vertex_positions.push_back(x0 * scale);
        _vertex_positions.push_back(y0 * scale);
        _vertex_positions.push_back(x1 * scale);
        _vertex_positions.push_back(y0 * scale);
        _vertex_positions.push_back(x1 * scale);
        _vertex_positions.push_back(y1 * scale);
        _vertex_positions.push_back(x0 * scale);
        _vertex_positions.push_back(y1 * scale);
        
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
			_size = get_param("scaleW", line, line_number);
			line.rest();
		}
		else if (id == "info")
		{
			//info face="Bitstream Vera Sans" size=60 bold=0 italic=0 charset="" unicode=0 stretchH=100 smooth=1 aa=1 padding=15,15,15,15 spacing=-2,-2
			get_string_param("face", line, line_number);
			get_param("size", line, line_number);
			get_param("bold", line, line_number);
			get_param("italic", line, line_number);
			get_string_param("charset", line, line_number);
			get_param("unicode", line, line_number);
			get_param("stretchH", line, line_number);
			get_param("smooth", line, line_number);
			get_param("aa", line, line_number);
			
			auto margin = get_margin_param("padding", line, line_number);
			auto spacing = get_margin_param("spacing", line, line_number);
			_advance_adjustment = margin.left + margin.right + spacing.left + spacing.right;
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

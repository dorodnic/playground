#pragma once

#include <fstream>
#include <streambuf>
#include <sstream>
#include <vector>
#include <algorithm>

#include "../rapidxml/rapidxml.hpp"
#include "../easyloggingpp/easylogging++.h"

#include "ui.h"

inline std::string to_lower(std::string data)
{
    std::transform(data.begin(), data.end(), data.begin(), ::tolower);
    return data;
}

class MinimalParser
{
public:
    MinimalParser(const std::string& line) : _line(line) {}
    
    std::string to_string() const
    {
        return "Parsing \"" + _line.substr(0, _index - 1) 
                + "|>>>" + _line.substr(_index - 1) + "\"";
    }
    
    bool eof() const { return _index >= _line.size(); }
    char get() { return _line[_index++]; }
    char peek() const { return _line[_index]; }
    void req(char c) 
    {
        if (get() != c)
        {
            std::stringstream ss; 
            ss << "'" << c << "' is expected! " << to_string();
            throw std::runtime_error(ss.str());
        }
    }
    void req_eof() 
    {
        if (!eof())
        {
            std::stringstream ss; 
            ss << "Expected end of line! " << to_string();
            throw std::runtime_error(ss.str());
        }
    }
    
    static bool is_digit(char c) { return c >= '0' && c <= '9'; }
    static bool is_letter(char c) 
    { 
        return (c >= 'a' && c <= 'z') ||
               (c >= 'A' && c <= 'Z') || (c == '_');
    }
    
    std::string get_id() 
    {
        auto d = get();
        if (!is_letter(d)) {
            std::stringstream ss; 
            ss << "Expected a letter! " << to_string();
            throw std::runtime_error(ss.str());
        }
        std::string result = &d;
        while (is_letter(peek()) || is_digit(peek()))
        {
            d = get();
            result += d;
        }
        return result;
    }
    
    int get_int() 
    {
        auto d = get();
        if (!is_digit(d)) {
            std::stringstream ss; 
            ss << "Expected a digit! " << to_string();
            throw std::runtime_error(ss.str());
        }
        int num = (d - '0');
        while (is_digit(peek()))
        {
            d = get();
            num = num * 10 + (d - '0');
        }
        return num;
    }
    
    template<typename T>
    T get_const_ids(std::map<std::string, T> map)
    {
        auto id = to_lower(get_id());
        
        auto it = map.find(id);
        if (it != map.end())
        {
            return it->second;   
        }

        std::stringstream ss; 
        ss << "Unknown Identifier '" << id << "'! " << to_string();
        throw std::runtime_error(ss.str());
    }
    
    Size get_size() 
    {
        if (peek() == '*')
        {
            get();
            return 1.0f;
        }
    
        if (is_letter(peek()))
        {
            return get_const_ids<Size>({ { "auto", 0 } });
        }
    
        auto x = get_int();
        if (peek() == '%')
        {
            get();
            return x / 100.0f;
        }
        else
        {
            return x;
        }
    }
    
    Size2 get_size2()
    {
        if (eof()) return { 0, 0 };
        
        if (is_letter(peek()))
        {
            return get_const_ids<Size2>({ { "auto", { 0, 0 } } });
        }
        
        auto first = peek();
        
        auto x = get_size();
        
        if (eof() && first == '*') return { 1.0f, 1.0f };
        
        req(',');
        auto y = get_size();
        req_eof();
        return { x, y };
    }
    
    Color3 get_color()
    {
        if (is_letter(peek()))
        {
            return get_const_ids<Color3>({ 
                    { "red",    { 1.0f, 0.0f, 0.0f } },
                    { "green",  { 0.0f, 1.0f, 0.0f } },
                    { "blue",   { 0.0f, 0.0f, 1.0f } },
                    { "white",  { 1.0f, 1.0f, 1.0f } },
                    { "black",  { 0.0f, 0.0f, 0.0f } },
                    { "gray",   { 0.5f, 0.5f, 0.5f } },
                    { "pink",   { 1.0f, 0.3f, 0.5f } },
                    { "yellow", { 1.0f, 1.0f, 0.0f } },
                    { "violet", { 0.7f, 0.0f, 1.0f } },
                });
        }
    
        if (eof()) return { 0.6f, 0.6f, 0.6f };
        auto r = get_int();
        req(',');
        auto g = get_int();
        req(',');
        auto b = get_int();
        req_eof();
        
        return { clamp(r / 255.0f, 0.0f, 1.0f),
                 clamp(g / 255.0f, 0.0f, 1.0f),
                 clamp(b / 255.0f, 0.0f, 1.0f) };
    }
    
    Margin get_margin()
    {
        if (eof()) return { 0 };
        auto left = get_int();
        if (eof()) return { left }; 
        
        if (peek() == '^')
        {
            get();
            req_eof();
            return { left, 0, left, left };
        }
        
        req(',');
        auto top = get_int();
        if (eof()) return { left, top };
        req(',');
        auto right = get_int();
        req(',');
        auto bottom = get_int();
        req_eof();
        
        return { left, top, right, bottom };
    }
    
private:
    std::string _line;
    int _index = 0;
};

typedef std::vector<rapidxml::xml_attribute<>*> AttrBag;

class Serializer
{
public:
    Serializer(const char* filename);
    
    std::shared_ptr<IVisualElement> deserialize()
    {
        AttrBag bag;
        return deserialize(_doc.first_node(), bag);
    }
private:
    std::shared_ptr<IVisualElement> deserialize(rapidxml::xml_node<>* node,
                                                const AttrBag& bag);

    Size2 parse_size(const std::string& str)
    {
        MinimalParser p(str);
        return p.get_size2();
    }

    Color3 parse_color(const std::string& str)
    {
        MinimalParser p(str);
        return p.get_color();
    }

    Margin parse_margin(const std::string& str)
    {
        MinimalParser p(str);
        return p.get_margin();
    }

    Orientation parse_orientation(const std::string& str)
    {
        if (str == "") return Orientation::vertical;
        
        auto s = to_lower(str);
        if (s == "vertical") return Orientation::vertical;
        if (s == "horizontal") return Orientation::horizontal;
        std::stringstream ss; ss << "Invalid Orientation '" << str << "'";
        throw std::runtime_error(ss.str());
    }
    
    Alignment parse_text_alignment(const std::string& str)
    {
        if (str == "") return Alignment::center;
        
        auto s = to_lower(str);
        if (s == "left") return Alignment::left;
        if (s == "center") return Alignment::center;
        if (s == "right") return Alignment::right;
        std::stringstream ss; ss << "Invalid Alignment '" << str << "'";
        throw std::runtime_error(ss.str());
    }
    
    std::vector<char> _buffer;
    rapidxml::xml_document<> _doc;
};

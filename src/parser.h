#pragma once

#include <sstream>
#include <vector>
#include <algorithm>

#include "../easyloggingpp/easylogging++.h"

#include "types.h"

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
        return "Parsing \"" + _line.substr(0, _index) 
                + "|>>>" + (_index <= _line.size() ? _line.substr(_index) : "") + "\"";
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
        char d[2];
        d[1] = 0;
        d[0] = get();
        if (!is_letter(d[0])) {
            std::stringstream ss; 
            ss << "Expected a letter! " << to_string();
            throw std::runtime_error(ss.str());
        }
        std::string result = d;
        while (is_letter(peek()) || is_digit(peek()))
        {
            d[0] = get();
            result += d;
        }
        return result;
    }
    
    std::string rest() const
    {
        return _line.substr(_index);
    }
    
    bool try_get_string(const std::string& prefix)
    {
        for (auto i = 0; i < prefix.size(); i++)
        {
            if (peek() == prefix[i]) get();
            else return false;
        }
        return true;
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
    
    float get_float() 
    {
        return get_int(); // TODO: fix!
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
        
        auto first = peek();
        
        auto x = get_size();
        
        if (eof() && x.is_auto()) return { x, x };
        
        if (eof() && first == '*') return { 1.0f, 1.0f };
        
        req(',');
        auto y = get_size();
        req_eof();
        return { x, y };
    }
    
    bool get_bool()
    {
        return get_const_ids<bool>({ 
                    { "true",  true },
                    { "false", false },
                });
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
    
    ~MinimalParser()
    {
        if (_index < _line.size())
            LOG(WARNING) << "Not everything was parsed! " << to_string();
    }
    
private:
    std::string _line;
    int _index = 0;
};

namespace type_string_traits
{
    template<class T>
    inline T parse(const std::string& str, T* output = nullptr);

    template<class T>
    inline std::string to_string(T val)
    {
        std::stringstream ss;
        ss << val;
        return ss.str();
    }

    template<>
    inline std::string to_string(std::string val)
    {
        return val;
    }

    template<>
    inline std::string to_string(bool val)
    {
        return val ? "true" : "false";
    }
    
    template<>
    inline std::string to_string(Orientation s)
    {
        if (s == Orientation::vertical) return "vertical";
        else if (s == Orientation::horizontal) return "horizontal";
        else return "unknown";
    }
    
    template<>
    inline std::string to_string(Alignment s)
    {
        if (s == Alignment::left) return "left";
        else if (s == Alignment::center) return "center";
        else if (s == Alignment::right) return "right";
        else return "unknown";
    }

    template<>
    inline int parse(const std::string& str, int*)
    {
        MinimalParser p(str);
        return p.get_int();
    }
    template<>
    inline float parse(const std::string& str, float*)
    {
        MinimalParser p(str);
        return p.get_float();
    }
    template<>
    inline bool parse(const std::string& str, bool*)
    {
        MinimalParser p(str);
        return p.get_bool();
    }
    template<>
    inline std::string parse(const std::string& str, std::string*)
    {
        return str;
    }
    template<>
    inline Size parse(const std::string& str, Size*)
    {
        MinimalParser p(str);
        return p.get_size();
    }
    template<>
    inline Size2 parse(const std::string& str, Size2*)
    {
        MinimalParser p(str);
        return p.get_size2();
    }
    template<>
    inline Color3 parse(const std::string& str, Color3*)
    {
        MinimalParser p(str);
        return p.get_color();
    }
    template<>
    inline Margin parse(const std::string& str, Margin*)
    {
        MinimalParser p(str);
        return p.get_margin();
    }
    
    template<>
    inline Orientation parse(const std::string& str, Orientation*)
    {
        if (str == "") return Orientation::vertical;
        
        auto s = to_lower(str);
        if (s == "vertical") return Orientation::vertical;
        if (s == "horizontal") return Orientation::horizontal;
        std::stringstream ss; ss << "Invalid Orientation '" << str << "'";
        throw std::runtime_error(ss.str());
    }
    
    template<>
    inline Alignment parse(const std::string& str, Alignment*)
    {
        if (str == "") return Alignment::center;
        
        auto s = to_lower(str);
        if (s == "left") return Alignment::left;
        if (s == "center") return Alignment::center;
        if (s == "right") return Alignment::right;
        std::stringstream ss; ss << "Invalid Alignment '" << str << "'";
        throw std::runtime_error(ss.str());
    }
    
    
    template<class T>
    inline std::string type_to_string(T* input);
    
    #define DECLARE_TYPE_NAME(T) template<>\
    inline std::string type_to_string(T* input) { return #T; }
    
    DECLARE_TYPE_NAME(int);
    DECLARE_TYPE_NAME(bool);
    DECLARE_TYPE_NAME(std::string);
    DECLARE_TYPE_NAME(float);
    DECLARE_TYPE_NAME(Size);
    DECLARE_TYPE_NAME(Size2);
    DECLARE_TYPE_NAME(Margin);
    DECLARE_TYPE_NAME(Color3);
    DECLARE_TYPE_NAME(Orientation);
    DECLARE_TYPE_NAME(Alignment);
};

std::string remove_prefix(const std::string& prefix, const std::string& str);
bool starts_with(const std::string& prefix, const std::string& str);

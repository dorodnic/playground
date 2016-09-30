#include "parser.h"

std::string remove_prefix(const std::string& prefix, const std::string& str)
{
    MinimalParser p(str);
    if (p.try_get_string(prefix))
    {
        auto rest = p.rest();
        while (!p.eof()) p.get();
        return rest;
    }
    else
    {
        while (!p.eof()) p.get();
        return str;
    }
}

bool starts_with(const std::string& prefix, const std::string& str)
{
    MinimalParser p(str);
    if (p.try_get_string(prefix))
    {
        while (!p.eof()) p.get();
        return true;
    }
    else
    {
        while (!p.eof()) p.get();
        return false;
    }
}

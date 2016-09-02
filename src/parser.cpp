#include "parser.h"

using namespace std;
using namespace rapidxml;

Serializer::Serializer(const char* filename)
{
    ifstream theFile(filename);
    _buffer = vector<char>((istreambuf_iterator<char>(theFile)), 
                                 istreambuf_iterator<char>());
    _buffer.push_back('\0');
    _doc.parse<0>(_buffer.data());
}

shared_ptr<IVisualElement> Serializer::deserialize(xml_node<>* node)
{
    string type = node->name();
    
    auto name_node = node->first_attribute("name");
    auto name = name_node ? name_node->value() : "";
    string name_str = name_node ? (string(" ") + name) : "";
    
    auto position_node = node->first_attribute("position");
    auto position_str = position_node ? position_node->value() : "";
    auto position = parse_size(position_str);
    
    auto size_node = node->first_attribute("size");
    auto size_str = size_node ? size_node->value() : "";
    auto size = parse_size(size_str);
    
    auto margin_node = node->first_attribute("margin");
    auto margin_str = margin_node ? margin_node->value() : "";
    auto margin = parse_margin(margin_str);
    
    if (type == "Button")
    {        
        auto color_node = node->first_attribute("color");
        auto color_str = color_node ? color_node->value() : "";
        auto color = parse_color(color_str);
        
        return shared_ptr<Button>(new Button(
            position, size, margin, color
        ));
    }
    else if (type == "Container")
    {       
        auto ori_node = node->first_attribute("orientation");
        auto ori_str = ori_node ? ori_node->value() : "";
        auto orientation = parse_orientation(ori_str);
         
        auto res = shared_ptr<Container>(new Container(
            position, size, margin, orientation
        ));
        
        for (xml_node<>* sub_node = node->first_node(); 
             sub_node; 
             sub_node = sub_node->next_sibling()) {
            try
            {
                res->add_item(deserialize(sub_node));
            } catch (const exception& ex) {
                LOG(ERROR) << "Parsing Error: " << ex.what() << " (" << type 
                           << name_str << ")" << endl;
            }
        }
        
        return res;
    }
    else if (type == "Grid")
    {       
        auto ori_node = node->first_attribute("orientation");
        auto ori_str = ori_node ? ori_node->value() : "";
        auto orientation = parse_orientation(ori_str);
         
        auto res = shared_ptr<Grid>(new Grid(
            position, size, margin, orientation
        ));
        
        for (xml_node<>* sub_node = node->first_node(); 
             sub_node; 
             sub_node = sub_node->next_sibling()) {
            try
            {
                string sub_name = sub_node->name();
                if (sub_name == "Break") res->commit_line();
                else res->add_item(deserialize(sub_node));
            } catch (const exception& ex) {
                LOG(ERROR) << "Parsing Error: " << ex.what() << " (" << type 
                           << name_str << ")" << endl;
            }
        }
        
        res->commit_line();
        
        return res;
    }
    
    stringstream ss; ss << "Unrecognized Visual Element (" << type 
                             << name_str << ")";
    throw runtime_error(ss.str());
}

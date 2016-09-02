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
    
    if (type == "TextBlock")
    {        
        auto color_node = node->first_attribute("text.color");
        auto color_str = color_node ? color_node->value() : "black";
        auto color = parse_color(color_str);

        auto align_node = node->first_attribute("text.align");
        auto align_str = align_node ? align_node->value() : "center";
        auto align = parse_text_alignment(align_str);
        
        auto txt_node = node->first_attribute("text");
        auto txt_str = txt_node ? txt_node->value() : "";
        
        return shared_ptr<TextBlock>(new TextBlock(
            name, txt_str, align, position, size, margin, color
        ));
    }
    else if (type == "Button")
    {        
        auto color_node = node->first_attribute("color");
        auto color_str = color_node ? color_node->value() : "gray";
        auto color = parse_color(color_str);
        
        auto txt_color_node = node->first_attribute("text.color");
        auto txt_color_str = txt_color_node ? txt_color_node->value() : "black";
        auto txt_color = parse_color(txt_color_str);
        
        auto align_node = node->first_attribute("text.align");
        auto align_str = align_node ? align_node->value() : "center";
        auto align = parse_text_alignment(align_str);
        
        auto txt_node = node->first_attribute("text");
        auto txt_str = txt_node ? txt_node->value() : "";
        
        return shared_ptr<Button>(new Button(
            name, txt_str, align, txt_color, position, size, margin, color
        ));
    }
    else if (type == "StackPanel")
    {       
        auto ori_node = node->first_attribute("orientation");
        auto ori_str = ori_node ? ori_node->value() : "";
        auto orientation = parse_orientation(ori_str);
         
        auto res = shared_ptr<StackPanel>(new StackPanel(
            name, position, size, margin, orientation
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
    else if (type == "Panel")
    {                
        auto res = shared_ptr<Panel>(new Panel(
            name, position, size, margin
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
            name, position, size, margin, orientation
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

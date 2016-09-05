#include "parser.h"

#include "controls.h"
#include "containers.h"
#include "adaptors.h"

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

xml_attribute<>* find_attribute(xml_node<>* node, const std::string& name,
                                const AttrBag& bag)
{
    auto attr = node->first_attribute(name.c_str());
    if (attr != nullptr) return attr;
    for (auto p : bag)
    {
        if (p->name() == name) return p;
    }
    return nullptr;
}

void Serializer::parse_container(Container* container, 
                                 xml_node<>* node,
                                 const std::string& name, 
                                 const AttrBag& bag)
{
    for (auto sub_node = node->first_node(); 
         sub_node; 
         sub_node = sub_node->next_sibling()) {
        try
        {
            container->add_item(deserialize(container, sub_node, bag));
        } catch (const exception& ex) {
            LOG(ERROR) << "Parsing Error: " << ex.what() << " (" << node->name() 
                       << name << ")" << endl;
        }
    }
}

shared_ptr<IVisualElement> Serializer::deserialize(IVisualElement* parent,
                                                   xml_node<>* node,
                                                   const AttrBag& bag)
{
    string type = node->name();
    shared_ptr<IVisualElement> res = nullptr;
    
    auto name_node = find_attribute(node, "name", bag);
    auto name = name_node ? name_node->value() : "";
    string name_str = name_node ? (string(" ") + name) : "";
    
    auto position_node = find_attribute(node, "position", bag);
    auto position_str = position_node ? position_node->value() : "";
    auto position = parse_size(position_str);
    
    auto size_node = find_attribute(node, "size", bag);
    auto size_str = size_node ? size_node->value() : "";
    auto size = parse_size(size_str);
    
    auto margin_node = find_attribute(node, "margin", bag);
    auto margin_str = margin_node ? margin_node->value() : "";
    auto margin = parse_margin(margin_str);
    
    auto align_node = find_attribute(node, "align", bag);
    auto align_str = align_node ? align_node->value() : "left";
    auto align = parse_text_alignment(align_str);
    
    auto txt_color_node = find_attribute(node, "text.color", bag);
    auto txt_color_str = txt_color_node ? txt_color_node->value() : "black";
    auto txt_color = parse_color(txt_color_str);

    auto txt_align_node = find_attribute(node, "text.align", bag);
    auto txt_align_str = txt_align_node ? txt_align_node->value() : "left";
    auto txt_align = parse_text_alignment(txt_align_str);
    
    auto txt_node = find_attribute(node, "text", bag);
    auto txt_str = txt_node ? txt_node->value() : "";
    
    auto color_node = find_attribute(node, "color", bag);
    auto color_str = color_node ? color_node->value() : "gray";
    auto color = parse_color(color_str);
    
    auto ori_node = find_attribute(node, "orientation", bag);
    auto ori_str = ori_node ? ori_node->value() : "";
    auto orientation = parse_orientation(ori_str);
    
    auto selected_node = find_attribute(node, "selected", bag);
    auto selected_str = selected_node ? selected_node->value() : "\\";

    if (type == "TextBlock")
    {                
        res = shared_ptr<TextBlock>(new TextBlock(
            name, txt_str, txt_align, position, size, 0, txt_color
        ));
    }
    else if (type == "Button")
    {
        if (find_attribute(node, "text.align", bag) == nullptr) 
            txt_align = Alignment::center;
    
        res = shared_ptr<Button>(new Button(
            name, txt_str, txt_align, txt_color, 
            align, position, size, 0, color
        ));
    }
    else if (type == "StackPanel")
    {
        auto panel = shared_ptr<StackPanel>(new StackPanel(
            name, position, size, 0, align, orientation, nullptr
        ));
        
        parse_container(panel.get(), node, name_str, bag);

        res = panel;
    }
    else if (type == "Panel")
    {                
        auto panel = shared_ptr<Panel>(new Panel(
            name, position, size, 0, align
        ));
        
        parse_container(panel.get(), node, name_str, bag);
         
        res = panel;
    }
    else if (type == "PageView")
    {                
        auto panel = shared_ptr<PageView>(new PageView(
            name, position, size, 0, align
        ));
        
        parse_container(panel.get(), node, name_str, bag);
        
        auto selected_item = panel->find_element(selected_str);
        panel->set_focused_child(selected_item);
        
        res = panel;
    }
    else if (type == "Grid")
    {
        auto grid = shared_ptr<Grid>(new Grid(
            name, position, size, 0, align, orientation
        ));
        
        for (auto sub_node = node->first_node(); 
             sub_node; 
             sub_node = sub_node->next_sibling()) {
            try
            {
                string sub_name = sub_node->name();
                if (sub_name == "Break") grid->commit_line();
                else grid->add_item(deserialize(grid.get(), sub_node, bag));
            } catch (const exception& ex) {
                LOG(ERROR) << "Parsing Error: " << ex.what() << " (" << type 
                           << name_str << ")" << endl;
            }
        }
        
        grid->commit_line();
        res = grid;
    }
    else if (type == "Using")
    {        
        auto sub_node = node->first_node();
        
        AttrBag new_bag;
        for (auto attr = node->first_attribute(); 
             attr; 
             attr = attr->next_attribute()) {
            new_bag.push_back(attr);
        }
        for (auto attr : bag)
        {
            auto it = std::find_if(new_bag.begin(), new_bag.end(),
                        [attr](xml_attribute<>* x) { 
                            return std::string(x->name()) == attr->name(); 
                        });
            if (it == new_bag.end())
            {
                new_bag.push_back(attr);
            }
        }
        
        try
        {
            res = deserialize(parent, sub_node, new_bag);
        } catch (const exception& ex) {
            LOG(ERROR) << "Parsing Error: " << ex.what() << " (" << type 
                       << name_str << ")" << endl;
        }
        
        if (sub_node->next_sibling())
        {
            stringstream ss; 
            ss << "Using should not have multiple nested items!";
            throw runtime_error(ss.str());
        }
    }
    
    if (!res)
    {
        stringstream ss; ss << "Unrecognized Visual Element (" << type 
                                 << name_str << ")";
        throw runtime_error(ss.str());
    }
    
    auto control = dynamic_cast<ControlBase*>(res.get());
    if (control) control->update_parent(parent);
    
    if (margin)
    {
        res = shared_ptr<MarginAdaptor>(new MarginAdaptor(
            res, margin
        ));
    }
    
    return res;
}

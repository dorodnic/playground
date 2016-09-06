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

std::string find_attribute(xml_node<>* node, const std::string& name,
                           const AttrBag& bag, const std::string& def)
{
    auto attr = node->first_attribute(name.c_str());
    if (attr != nullptr) return attr->value();
    for (auto p : bag)
    {
        if (p->name() == name) return p->value();
    }
    return def;
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
    
    auto name = find_attribute(node, "name", bag, "");
    
    auto position_str = find_attribute(node, "position", bag, "");
    auto position = parse_size(position_str);
    
    auto size_str = find_attribute(node, "size", bag, "auto");
    auto size = parse_size(size_str);
    
    auto margin_str = find_attribute(node, "margin", bag, "0");
    auto margin = parse_margin(margin_str);
    
    auto align_str = find_attribute(node, "align", bag, "left");
    auto align = parse_text_alignment(align_str);
    
    auto txt_color_str = find_attribute(node, "text.color", bag, "black");
    auto txt_color = parse_color(txt_color_str);

    auto txt_align_str = find_attribute(node, "text.align", bag, "left");
    auto txt_align = parse_text_alignment(txt_align_str);
    
    auto txt_str = find_attribute(node, "text", bag, "");

    auto color_str = find_attribute(node, "color", bag, "gray");
    auto color = parse_color(color_str);
    
    auto ori_str = find_attribute(node, "orientation", bag, "vertical");
    auto orientation = parse_orientation(ori_str);

    auto selected_str = find_attribute(node, "selected", bag, "\\");
    
    auto visible_str = find_attribute(node, "visible", bag, "true");
    auto visible = parse_bool(visible_str);

    auto enabled_str = find_attribute(node, "enabled", bag, "true");
    auto enabled = parse_bool(enabled_str);

    if (type == "TextBlock")
    {                
        res = shared_ptr<TextBlock>(new TextBlock(
            name, txt_str, txt_align, position, size, txt_color
        ));
    }
    else if (type == "Button")
    {
        if (find_attribute(node, "text.align", bag, "\\") == "\\") 
            txt_align = Alignment::center; // override default for buttons
    
        res = shared_ptr<Button>(new Button(
            name, txt_str, txt_align, txt_color, 
            align, position, size, color
        ));
    }
    else if (type == "Slider")
    {
        auto min_str = find_attribute(node, "min", bag, "0");
        auto max_str = find_attribute(node, "max", bag, "100");
        auto step_str = find_attribute(node, "step", bag, "20");
        auto value_str = find_attribute(node, "value", bag, "0");
        
        auto min = parse_float(min_str);
        auto max = parse_float(max_str);
        auto step = parse_float(step_str);
        auto value = parse_float(value_str);
    
        res = shared_ptr<Slider>(new Slider(
            name, align, position, size, color, txt_color, 
            orientation, min, max, step, value
        ));
    }
    else if (type == "StackPanel")
    {
        auto panel = shared_ptr<StackPanel>(new StackPanel(
            name, position, size, align, orientation, nullptr
        ));
        
        parse_container(panel.get(), node, name, bag);

        res = panel;
    }
    else if (type == "Panel")
    {                
        auto panel = shared_ptr<Panel>(new Panel(
            name, position, size, align
        ));
        
        parse_container(panel.get(), node, name, bag);
         
        res = panel;
    }
    else if (type == "PageView")
    {                
        auto panel = shared_ptr<PageView>(new PageView(
            name, position, size, align
        ));
        
        parse_container(panel.get(), node, name, bag);
        
        auto selected_item = panel->find_element(selected_str);
        panel->set_focused_child(selected_item);
        
        res = panel;
    }
    else if (type == "Grid")
    {
        auto grid = shared_ptr<Grid>(new Grid(
            name, position, size, align, orientation
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
                           << " " << name << ")" << endl;
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
                       << " " << name << ")" << endl;
        }
        
        if (sub_node->next_sibling())
        {
            stringstream ss; 
            ss << "Using should not have multiple nested items!";
            throw runtime_error(ss.str());
        }
    }
    
    // update controls parent before applying any adaptors
    auto control = dynamic_cast<ControlBase*>(res.get());
    if (control) control->update_parent(parent);
    
    if (!res)
    {
        stringstream ss; ss << "Unrecognized Visual Element (" << type 
                            << " " << name << ")";
        throw runtime_error(ss.str());
    }

    if (margin)
    {
        res = shared_ptr<MarginAdaptor>(new MarginAdaptor(
            res, margin
        ));
    }
    
    if (!visible) LOG(INFO) << "element " << name << " is invisible!";
    
    res = shared_ptr<VisibilityAdaptor>(new VisibilityAdaptor(
            res, visible
        ));
        
    res->set_enabled(enabled);
    
    return res;
}

#include "serializer.h"
#include "controls.h"
#include "containers.h"
#include "adaptors.h"

#include <fstream>
#include <streambuf>

using namespace std;
using namespace rapidxml;


struct BindingBridge : public BindableObjectBase
{
    bool object_exists = false;
    
    std::shared_ptr<Binding> binding;
};

template<>
struct TypeDefinition<BindingBridge>
{
    static std::shared_ptr<ITypeDefinition> make() 
    {
        DefineClass(BindingBridge)->AddField(object_exists);
    }
};

class BridgeConverter : public TypeConverterBase<
        std::shared_ptr<INotifyPropertyChanged>, bool>
{
public:
    BridgeConverter(std::function<void(std::shared_ptr<INotifyPropertyChanged> x)> created,
                    std::function<void()> destroyed)
        : _created(created), _destroyed(destroyed)
    {
    }

    bool convert(std::shared_ptr<INotifyPropertyChanged> x) const override
    {
        bool is_null = !x.get();
        if (_was_null != is_null)
        {
            if (is_null) _destroyed();
            else _created(x);
            _was_null = is_null;
        }
        
        if (x.get() != last)
        {
            _destroyed();
            _created(x);
            last = x.get();
        }
    
        return x.get();
    }
    std::shared_ptr<INotifyPropertyChanged> convert(bool x) const override
    {
        return nullptr;
    }
    
private:
    mutable bool _was_null = true;
    mutable INotifyPropertyChanged* last = nullptr;
    std::function<void(std::shared_ptr<INotifyPropertyChanged> x)> _created;
    std::function<void()> _destroyed;
};

class ExtendedBinding : public Binding
{
public:
    ExtendedBinding(TypeFactory& factory,
        std::weak_ptr<INotifyPropertyChanged> a, std::string a_prop,
        std::weak_ptr<INotifyPropertyChanged> b, std::string b_prop,
        std::unique_ptr<ITypeConverter> converter,
        std::shared_ptr<BindingBridge> bridge)
        : Binding(factory, a, a_prop, b, b_prop, std::move(converter)),
          _bridge(bridge)
    {
    }
    
private:
    std::shared_ptr<BindingBridge> _bridge;
};

std::shared_ptr<INotifyPropertyChanged> Serializer::deserialize()
{
    if (!_factory.find_type("BindingBridge"))
    {
        _factory.register_type<BindingBridge>();
    }

    AttrBag bag;
    BindingBag bindings;
    ElementsMap elements;
    auto res = deserialize(nullptr, _doc.first_node(), bag, bindings, elements);
    auto elem = dynamic_cast<IVisualElement*>(res.get());
    for (auto& def : bindings)
    {
        auto a_ptr = elements[def.a];
        auto a = dynamic_cast<ControlBase*>(def.a);
        auto b_ptr = elements[elem->find_element(def.b_name)];
        
        std::shared_ptr<Binding> binding;
        MinimalParser p(def.b_prop);
        auto prop_name = p.get_id();
        if (p.peek() == '.')
        {
            p.get();
            auto prop2_name = p.get_id();
            
            p.req_eof();
            
            std::shared_ptr<BindingBridge> bridge(new BindingBridge());
            std::weak_ptr<BindingBridge> weak(bridge);
           
            auto on_create = [this, weak, a, a_ptr, def, prop2_name]
                (std::shared_ptr<INotifyPropertyChanged> x)
            {
                auto b = weak.lock();
                if (b)
                {
                    b->binding.reset(new Binding(
                        _factory, a_ptr, def.a_prop, x, prop2_name));
                    if (a) a->add_binding(b->binding);
                }
            };
            auto on_delete = [this, weak, a](){
                auto b = weak.lock();
                if (b)
                {
                    if (a) a->remove_binding(b->binding);
                    b->binding = nullptr;
                }
            };
            
            std::unique_ptr<ITypeConverter> converter(
                new BridgeConverter(on_create, on_delete));
            
            binding.reset(new ExtendedBinding(
                _factory, bridge, "object_exists", b_ptr, prop_name, 
                std::move(converter), bridge));
        }
        else
        {
            binding.reset(new Binding(
                _factory, a_ptr, def.a_prop, b_ptr, def.b_prop));
        }

        if (a) a->add_binding(binding);
    }
    return res;
}

Serializer::Serializer(const char* filename, TypeFactory& factory)
    : _factory(factory)
{
    ifstream theFile(filename);
    _buffer = vector<char>((istreambuf_iterator<char>(theFile)), 
                                 istreambuf_iterator<char>());
    _buffer.push_back('\0');
    _doc.parse<0>(_buffer.data());
}

std::string find_attribute(xml_node<>* node, const std::string& name,
                           const AttrBag& bag)
{
    auto attr = node->first_attribute(name.c_str());
    if (attr != nullptr) return attr->value();
    for (auto p : bag)
    {
        if (p->name() == name) return p->value();
    }
    return "";
}

bool has_attribute(xml_node<>* node, const std::string& name, 
                   const AttrBag& bag)
{
    auto attr = node->first_attribute(name.c_str());
    if (attr != nullptr) return true;
    for (auto p : bag)
    {
        if (p->name() == name) return true;
    }
    return false;
}

void Serializer::parse_container(Container* container, 
                                 xml_node<>* node,
                                 const std::string& name, 
                                 const AttrBag& bag,
                                 BindingBag& bindings,
                                 ElementsMap& elements)
{
    for (auto sub_node = node->first_node(); 
         sub_node; 
         sub_node = sub_node->next_sibling()) {
        try
        {
            string sub_name = sub_node->name();
            auto grid = dynamic_cast<Grid*>(container);
            if (sub_name == "Break" && grid) grid->commit_line();
            else container->add_item(
                deserialize(container, sub_node, bag, bindings, elements));
        } catch (const exception& ex) {
            LOG(ERROR) << "Parsing Error: " << ex.what() << " (" << node->name() 
                       << " " << name << ")" << endl;
        }
    }
}

void parse_binding(const std::string& prop_text, 
                   IPropertyDefinition* p_def,
                   INotifyPropertyChanged* ptr,
                   BindingBag& bindings)
{
    MinimalParser p(prop_text);
    p.try_get_string("{bind ");
    auto element_id = p.get_id();
    p.req('.');
    auto prop_name = p.get_id();
    while (p.peek() == '.')
    {
        p.get();
        prop_name += "." + p.get_id();
    }
    p.req('}');
    p.req_eof();
    LOG(INFO) << "Parsing binding to " << element_id << "." << prop_name;
    BindingDef binding
        {
            ptr,
            p_def->get_name(), 
            element_id,
            prop_name
        };
    bindings.push_back(binding);
}

shared_ptr<INotifyPropertyChanged> Serializer::deserialize(
                                                    IVisualElement* parent,
                                                    xml_node<>* node,
                                                    const AttrBag& bag,
                                                    BindingBag& bindings,
                                                    ElementsMap& elements)
{
    string type = node->name();
    auto name = find_attribute(node, "name", bag);
    
    if (type == "Using")
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
        
        if (sub_node->next_sibling())
        {
            stringstream ss; 
            ss << "Using should not have multiple nested items!";
            throw runtime_error(ss.str());
        }
        
        try
        {
            return deserialize(parent, sub_node, new_bag, bindings, elements);
        } catch (const exception& ex) {
            LOG(ERROR) << "Parsing Error: " << ex.what() << " (" << type 
                       << " " << name << ")" << endl;
        }
    }
   
    auto t_def = _factory.find_type(type);
    if (!t_def)
    {
        stringstream ss; ss << "Unrecognized Visual Element (" << type 
                            << " " << name << ")";
        throw runtime_error(ss.str());
    }
    
    shared_ptr<INotifyPropertyChanged> res = t_def->default_construct();
    
    for (auto prop_name : t_def->get_property_names())
    {
        auto p_def = t_def->get_property(prop_name);
        if (has_attribute(node, p_def->get_name(), bag))
        {
            auto prop_text = find_attribute(node, p_def->get_name(), bag);
            
            if (starts_with("{bind ", prop_text))
            {
                parse_binding(prop_text, p_def, res.get(), bindings);
            }
            else
            {
                auto prop = p_def->create(res);
                prop->set_value(prop_text);
            }
        }
    }
    
    // TODO: Run over all attributes and report unused ones
    
    auto panel = dynamic_cast<Container*>(res.get());
    if (panel)
    {
        std::unique_ptr<BindingDef> binding;
        
        auto grid = dynamic_cast<Grid*>(res.get());
        if (grid) grid->commit_line();
        
        parse_container(panel, node, name, bag, bindings, elements);
        
        if (grid) grid->commit_line();
    }
    
    elements[res.get()] = res; // update address mapping before applying adaptors
    
    // update controls parent before applying any adaptors
    auto control = dynamic_cast<ControlBase*>(res.get());
    if (control) 
    {
        control->update_parent(parent);
        
        auto margin_str = find_attribute(node, "margin", bag);
        margin_str = (margin_str == "" ? "0" : margin_str);
        auto margin = type_string_traits::parse(margin_str, (Margin*)nullptr);
        res = shared_ptr<MarginAdaptor>(
            new MarginAdaptor(res, margin));

        res = shared_ptr<VisibilityAdaptor>(
            new VisibilityAdaptor(res));
    }
    
    /*auto name = find_attribute(node, "name", bag, "");
    
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
    
    std::unique_ptr<BindingDef> binding;
    if (starts_with("{bind ", txt_str))
    {
        MinimalParser p(txt_str);
        p.try_get_string("{bind ");
        auto element_id = p.get_id();
        p.req('.');
        auto prop_name = p.get_id();
        p.req('}');
        p.req_eof();
        LOG(INFO) << element_id << " " << prop_name;
        binding.reset(new BindingDef 
            {
                nullptr, // a is unknown at this point, will be filled in later
                "text", // for now, we know exactly what attribute has the binding
                element_id,
                prop_name
            });
        txt_str = "";
    }

    auto color_str = find_attribute(node, "color", bag, "gray");
    auto color = parse_color(color_str);
    
    auto ori_str = find_attribute(node, "orientation", bag, "vertical");
    auto orientation = parse_orientation(ori_str);

    auto selected_str = find_attribute(node, "selected", bag, "\\");
    
    auto visible_str = find_attribute(node, "visible", bag, "true");
    auto visible = parse_bool(visible_str);

    auto enabled_str = find_attribute(node, "enabled", bag, "true");
    auto enabled = parse_bool(enabled_str);*/

    /*if (type == "TextBlock")
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
        
        parse_container(panel.get(), node, name, bag, bindings);

        res = panel;
    }
    else if (type == "Panel")
    {                
        auto panel = shared_ptr<Panel>(new Panel(
            name, position, size, align
        ));
        
        parse_container(panel.get(), node, name, bag, bindings);
         
        res = panel;
    }
    else if (type == "PageView")
    {                
        auto panel = shared_ptr<PageView>(new PageView(
            name, position, size, align
        ));
        
        parse_container(panel.get(), node, name, bag, bindings);
        
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
                else grid->add_item(deserialize(grid.get(), sub_node, bag, bindings));
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
            res = deserialize(parent, sub_node, new_bag, bindings);
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
    }*/
    
    return res;
}

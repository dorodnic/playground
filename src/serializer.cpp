#include "serializer.h"
#include "controls.h"
#include "containers.h"
#include "adaptors.h"
#include "nonvisual.h"
#include "font.h"

#include <fstream>
#include <streambuf>
#include <iterator>

using namespace std;
using namespace rapidxml;

struct BindingDef
{
    INotifyPropertyChanged* a;
    string a_prop;
    string b_name;
    string b_prop;
    string converter_name;
    BindingMode mode;
};

template<class T, class S>
class Dictionary : public TypeConverterBase<T, S>,
                   public BindableObjectBase
{
public:
    Dictionary() {}

    void add_key_value(const T& key, const S& value)
    {
        _map[key] = value;
        _reverse[value] = key;
    }

    S convert_to(T key) const override
    {
        return _map.find(key)->second;
    }

    T convert_from(S val) const override
    {
        return _reverse.find(val)->second;
    }

    S next_s(S key) const
    {
        auto it = _reverse.lower_bound(key);
        if (it == _reverse.end()) return std::prev(_reverse.end())->first;
        return it->first;
    }

    S prev_s(S key) const
    {
        auto it = _reverse.lower_bound(key);
        if (it == _reverse.begin()) return _reverse.begin()->first;
        return std::prev(it)->first;
    }

    T next_t(T key) const
    {
        auto it = _map.lower_bound(key);
        if (it == _map.end()) return std::prev(_map.end())->first;
        return it->first;
    }

    T prev_t(T key) const
    {
        auto it = _map.lower_bound(key);
        if (it == _map.begin()) return _map.begin()->first;
        return std::prev(it)->first;
    }

private:
    std::map<T, S> _map;
    std::map<S, T> _reverse;
};

class FloatDictionary : public Dictionary<float, float>
{
public:
    float convert_to(float key) const override
    {
        auto p = prev_t(key);
        auto n = next_t(key);
        auto t = (n != p) ? (key - p) / (n - p) : 0;
        auto pv = Dictionary<float, float>::convert_to(p);
        auto nv = Dictionary<float, float>::convert_to(n);
        auto v = pv + (nv - pv) * t;
        LOG(INFO) << "[" << p << " " << key << " " << n << "] = " << v;
        return v;
    }

    float convert_from(float val) const override
    {
        auto p = prev_s(val);
        auto n = next_s(val);
        auto t = (n != p) ? (val - p) / (n - p) : val;
        auto pv = Dictionary<float, float>::convert_from(p);
        auto nv = Dictionary<float, float>::convert_from(n);
        auto v = pv + (nv - pv) * t;
        LOG(INFO) << "[" << p << " " << val << " " << n << "] = " << v;
        return v;
    }

    bool interpolate = true;
};

template<>
struct TypeDefinition<FloatDictionary>
{
    static std::shared_ptr<ITypeDefinition> make()
    {
        DefineClass(FloatDictionary)
                ->AddField(interpolate);
    }
};

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

struct BindingOwner : public BindableObjectBase
{
    std::shared_ptr<INotifyPropertyChanged> object;
};

template<>
struct TypeDefinition<BindingOwner>
{
    static std::shared_ptr<ITypeDefinition> make() 
    {
        DefineClass(BindingOwner)->AddField(object);
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

    bool convert_to(std::shared_ptr<INotifyPropertyChanged> x) const override
    {
        bool is_null = !x.get();
        
        if (x.get() != last)
        {
            if (last) _destroyed();
            if (x.get()) _created(x);
            last = x.get();
        }
    
        return x.get();
    }
    std::shared_ptr<INotifyPropertyChanged> convert_from(bool x) const override
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
    ExtendedBinding(std::shared_ptr<TypeFactory> factory,
        std::weak_ptr<INotifyPropertyChanged> a, std::string a_prop,
        std::weak_ptr<INotifyPropertyChanged> b, std::string b_prop,
        std::shared_ptr<ITypeConverter> converter,
        BindingMode mode,
        std::shared_ptr<BindingBridge> bridge)
        : Binding(factory, a, a_prop, b, b_prop, mode, converter),
          _bridge(bridge)
    {
    }
    
private:
    std::shared_ptr<BindingBridge> _bridge;
};

class OwningBinding : public Binding
{
public:
    OwningBinding(std::shared_ptr<TypeFactory> factory,
        std::weak_ptr<INotifyPropertyChanged> a, std::string a_prop,
        std::weak_ptr<INotifyPropertyChanged> b, std::string b_prop,
        std::shared_ptr<BindingOwner> owner,
        BindingMode mode,
        std::shared_ptr<ITypeConverter> converter)
        : Binding(factory, a, a_prop, b, b_prop, mode, converter),
          _owner(owner)
    {
    }
    
private:
    std::shared_ptr<BindingOwner> _owner;
};

std::unique_ptr<Binding> create_multilevel_binding(
    std::shared_ptr<TypeFactory> factory,
    std::shared_ptr<INotifyPropertyChanged> a_ptr,
    std::shared_ptr<INotifyPropertyChanged> b_ptr,
    const std::string& a_prop,
    const std::string& b_prop,
    BindingMode mode,
    std::shared_ptr<ITypeConverter> converter
    )
{
    std::unique_ptr<Binding> binding;
    MinimalParser p(b_prop);
    auto prop_name = p.get_id();
    if (p.peek() == '.')
    {
        p.get();
        auto prop_rest = p.rest();
        p.req_eof();
        
        std::shared_ptr<BindingBridge> bridge(new BindingBridge());
        std::weak_ptr<BindingBridge> weak(bridge);
       
        auto on_create = [factory, weak, a_ptr, mode,
                          a_prop, prop_rest, converter](auto x)
        {
            auto b = weak.lock();
            if (b)
            {
                b->binding = create_multilevel_binding(
                    factory, a_ptr, x, a_prop,
                    prop_rest, mode, converter);
            }
        };
        auto on_delete = [weak](){
            auto b = weak.lock();
            if (b)
            {
                b->binding = nullptr;
            }
        };
        
        std::unique_ptr<ITypeConverter> converter(
            new BridgeConverter(on_create, on_delete));
        
        binding.reset(new ExtendedBinding(
            factory, bridge, "object_exists", b_ptr, prop_name, 
            std::move(converter), BindingMode::OneWay, bridge));
    }
    else
    {
        binding.reset(new Binding(factory, a_ptr, a_prop,
                                  b_ptr, b_prop, mode, converter));
    }
    return binding;
}

std::shared_ptr<INotifyPropertyChanged> Serializer::deserialize()
{
    if (!_factory.get())
    {
        _factory = make_shared<TypeFactory>();
	    _factory->register_type(TypeCollection<
	        Button, 
	        TextBlock, 
	        Slider,
	        Panel,
	        StackPanel,
	        Grid,
	        PageView,
	        Timer,
            Font,
            FloatDictionary
	        >());
    }

    if (!_factory->find_type("BindingBridge"))
    {
        _factory->register_type<BindingBridge>();
    }
    if (!_factory->find_type("BindingOwner"))
    {
        _factory->register_type<BindingOwner>();
    }


    AttrBag bag;
    BindingBag bindings;
    ElementsMap elements;
    auto res = deserialize(nullptr, _doc.first_node(), bag, bindings, elements);
    auto elem = dynamic_cast<IVisualElement*>(res.get());
    for (auto& def : bindings)
    {
        auto converter_obj = elem->find_element(def.converter_name);
        auto a_ptr = elements[def.a];
        auto a = dynamic_cast<ControlBase*>(def.a);
        auto b_obj = elem->find_element(def.b_name);
        std::shared_ptr<INotifyPropertyChanged> b_ptr, converter_ptr;
        if (b_obj)
        {
            b_ptr = elements[b_obj];
            auto adaptor = dynamic_cast<VisualAdaptor*>(b_ptr.get());
            if (adaptor) b_ptr = adaptor->get();
        }
        else
        {
            // assuming def.b_name is refering to a property of this
            if (def.b_name != "this")
            {
                if (def.b_prop != "")
                    def.b_prop = def.b_name + "." + def.b_prop;
                else
                    def.b_prop = def.b_name;
            }
            b_ptr = a_ptr;
            def.b_name = a->to_string();
        }
        
        LOG(INFO) << "Creating binding to " << a->to_string() << "." 
            << def.a_prop << " from " << def.b_name << "." << def.b_prop;
        if (converter_obj)
        {
            converter_ptr = elements[converter_obj];
            auto adaptor = dynamic_cast<VisualAdaptor*>(converter_ptr.get());
            if (adaptor) converter_ptr = adaptor->get();
        }
            
        if (def.b_prop == "")
        {
            auto owner = std::make_shared<BindingOwner>();
            owner->object = b_ptr;
            
            std::unique_ptr<Binding> binding(new OwningBinding(_factory, 
                                             a_ptr, def.a_prop, 
                                             owner, "object", owner, def.mode,
                                             std::dynamic_pointer_cast<ITypeConverter>(converter_ptr)));
                        
            if (a) a->add_binding(std::move(binding));
        }
        else
        {
            auto binding = create_multilevel_binding(
                        _factory, a_ptr, b_ptr, def.a_prop, def.b_prop, def.mode,
                        std::dynamic_pointer_cast<ITypeConverter>(converter_ptr));
            if (a) a->add_binding(std::move(binding));
        }
    }
    return res;
}

Serializer::Serializer(const char* filename, 
                       std::shared_ptr<TypeFactory> factory)
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
    
    std::string prop_name = "";
    std::string converter_name = "|";
    auto mode = BindingMode::TwoWay;
    
    if (p.peek() == '.')
    {
        p.req('.');
        prop_name = p.get_id();
        while (p.peek() == '.')
        {
            p.get();
            prop_name += "." + p.get_id();
        }
    }

    while (p.peek() == ',')
    {
        p.req(',');
        while (p.peek() == ' ') p.req(' ');
        auto id = p.get_id();
        if (id == "converter")
        {
            p.req('=');
            auto id = p.get_id();
            if (converter_name != "|")
            {
                throw std::runtime_error("Can't specify more then one converter!");
            }
            converter_name = id;
        }
        else if (id == "mode")
        {
            p.req('=');
            auto mode_str = to_lower(p.get_id());
            if (mode_str == "oneway") mode = BindingMode::OneWay;
            else if (mode_str == "twoway") mode = BindingMode::TwoWay;
            else if (mode_str == "onetime") mode = BindingMode::OneTime;
            else throw std::runtime_error(
                        str() << "Not supported binding mode " << mode_str);

        }
    }
    
    p.req('}');
    p.req_eof();
    LOG(INFO) << "Parsing binding to " << element_id << "." << prop_name;
    BindingDef binding
        {
            ptr,
            p_def->get_name(), 
            element_id,
            prop_name,
            converter_name,
            mode
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
   
    auto t_def = _factory->find_type(type);
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

    auto dict = dynamic_cast<FloatDictionary*>(res.get());
    if (dict)
    {
        auto sub_node = node->first_node();
        while (sub_node)
        {
            if (std::string(sub_node->name()) != "kvp")
                throw std::runtime_error("Only key-value-pairs (kvp) are allowed inside a dictionary!");

            auto key_attr = sub_node->first_attribute("key");
            auto value_attr = sub_node->first_attribute("value");
            if (!key_attr || !value_attr)
                throw std::runtime_error("kvp must have key and value attributes!");

            auto key_str = key_attr->value();
            auto value_str = value_attr->value();

            MinimalParser key_parser(key_str);
            auto key = key_parser.get_float();

            MinimalParser value_parser(value_str);
            auto value = value_parser.get_float();

            dict->add_key_value(key, value);

            sub_node = sub_node->next_sibling();
        }
    }
    
    auto control = dynamic_cast<ControlBase*>(res.get());
    
    if (!control)
    {
        res = shared_ptr<VisualAdaptor>(new VisualAdaptor(res, name));
    }
    
    elements[res.get()] = res; // update address mapping before applying adaptors
    
    if (control) 
    {
        control->update_parent(parent);
        
        auto margin_str = find_attribute(node, "margin", bag);
        margin_str = (margin_str == "" ? "0" : margin_str);
        auto margin = type_string_traits::parse(margin_str, (Margin*)nullptr);
        res = shared_ptr<MarginAdaptor>(new MarginAdaptor(res, margin));

        res = shared_ptr<VisibilityAdaptor>(new VisibilityAdaptor(res));
    }
    
    return res;
}

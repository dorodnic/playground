#pragma once

#include <functional>
#include <exception>
#include <memory>
#include <string>
#include <map>
#include <typeinfo>
#include <type_traits>
#include <typeindex>

#include "parser.h"
#include "types.h"

class IProperty;

typedef std::function<void(IProperty* prop)> 
        OnPropertyChangeCallback;
        
typedef std::function<void(const char* field_name)> 
        OnFieldChangeCallback;

class IVirtualBase
{
public:
    virtual ~IVirtualBase() {}
};

class IProperty : public IVirtualBase
{
public:
    virtual void set_value(std::string value) = 0;
    virtual std::string get_value() const = 0;
    
    virtual void subscribe_on_change(void* owner, 
                                     OnPropertyChangeCallback on_change) = 0;
    virtual void unsubscribe_on_change(void* owner) = 0;
};

class INotifyPropertyChanged;

class IPropertyDefinition : public IVirtualBase
{
public:
    virtual std::unique_ptr<IProperty> create
        (std::weak_ptr<INotifyPropertyChanged> owner) const = 0;
    
    virtual const std::string& get_type() const = 0;
    virtual const std::string& get_name() const = 0;
    
    virtual bool is_writable() const = 0;
};

class ITypeDefinition : public IVirtualBase
{
public:
    virtual const std::string& get_type() const = 0;

    virtual const IPropertyDefinition* get_property(const std::string& name) const = 0;
    virtual IPropertyDefinition* get_property(const std::string& name) = 0;
    virtual const std::vector<std::string> get_property_names() const = 0;
    
    virtual std::unique_ptr<INotifyPropertyChanged> default_construct() const = 0;
};

class INotifyPropertyChanged : public IVirtualBase
{
public:
    virtual void fire_property_change(const char* prop) = 0;

    virtual void subscribe_on_change(void* owner, 
                                     OnFieldChangeCallback on_change) = 0;
    virtual void unsubscribe_on_change(void* owner) = 0;
};

template<class T>
struct TypeDefinition
{
    std::shared_ptr<ITypeDefinition> make();
};

template<class... T>
struct TypeCollection
{
};

class TypeFactory
{
public:
    TypeFactory() {}
    
    template<class T>
    void register_type() 
    {
        auto ptr = TypeDefinition<T>::make();
        _types.push_back(ptr);
        _name_to_type[ptr->get_type()] = ptr.get();
        _typeid_to_type[typeid(T)] = ptr.get();
    }
    
    template<class T, class... S>
    void register_type(TypeCollection<T, S...> c) 
    {
        register_type<T>();
        register_type(TypeCollection<S...>());
    }
    
    template<class T>
    void register_type(TypeCollection<T> c) 
    {
        register_type<T>();
    }
    
    ITypeDefinition* find_type(const std::string& name)
    {
        auto it = _name_to_type.find(name);
        if (it != _name_to_type.end())
            return it->second;
        else
            return nullptr;
    }
    
    template<class T>
    ITypeDefinition* get_type_of(T* ptr)
    {
        if (ptr)
        {
            auto it = _typeid_to_type.find(std::type_index(typeid(*ptr)));
            if (it != _typeid_to_type.end())
                return it->second;
            else
                return nullptr;
        }
        else
        {
            auto it = _typeid_to_type.find(std::type_index(typeid(T)));
            if (it != _typeid_to_type.end())
                return it->second;
            else
                return nullptr;
        }
    }
    
private:
    std::vector<std::shared_ptr<ITypeDefinition>> _types;
    std::unordered_map<std::string, ITypeDefinition*> _name_to_type;
    std::unordered_map<std::type_index, ITypeDefinition*> _typeid_to_type;
};

class BindableObjectBase : public INotifyPropertyChanged
{
public:
    BindableObjectBase();
    
    void fire_property_change(const char* property_name) override;
    void subscribe_on_change(void* owner, 
                             OnFieldChangeCallback on_change) override;
    void unsubscribe_on_change(void* owner) override;
    
private:
    std::map<void*,OnFieldChangeCallback> _on_change;
};

class ICopyable : public IVirtualBase
{
public:
    virtual void copy_to(ICopyable* to) = 0;
    virtual void copy_from(ICopyable* from) = 0;
};

template<class S>
class PropertyBase : public IProperty, public ICopyable
{
public:

    PropertyBase(std::weak_ptr<INotifyPropertyChanged> owner, 
                 const std::string& name) 
        : _owner(owner), _on_change()
    {
        auto s = owner.lock();
        if (s)
        {
            s->subscribe_on_change(this,[this, name](const char* field){
                if (field == name)
                {
                    for(auto& evnt : _on_change)
                    {
                        evnt.second(this);
                    }
                }
            });
        }
    }
    
    ~PropertyBase()
    {
        auto s = _owner.lock();
        if (s) s->unsubscribe_on_change(this);
    }
    
    void copy_to(ICopyable* to) override
    {
        auto p = static_cast<PropertyBase<S>*>(to);
        p->set(get());
    }
    void copy_from(ICopyable* from) override
    {
        auto p = static_cast<PropertyBase<S>*>(from);
        set(p->get());
    }
    
    virtual void set(S val) = 0;
    virtual S get() const = 0;
    
    void set_value(std::string value) override
    {
        auto s = type_string_traits::parse(value, (S*)(nullptr));
        set(s);
        for(auto& evnt : _on_change)
        {
            evnt.second(this);
        }
    }
    std::string get_value() const override
    {
        return type_string_traits::to_string(get());
    }
    
    void subscribe_on_change(void* owner, 
                             OnPropertyChangeCallback on_change) override
    {
        _on_change[owner] = on_change;
    }
    void unsubscribe_on_change(void* owner) override
    {
        _on_change.erase(owner);
    }
    
private:
    std::map<void*,OnPropertyChangeCallback> _on_change;
    std::weak_ptr<INotifyPropertyChanged> _owner;
};

typedef std::function<IProperty*(std::weak_ptr<INotifyPropertyChanged>, const std::string&)> 
        PropertyCreator;

template<class S>
class PropertyDefinition : public IPropertyDefinition
{
public:
    PropertyDefinition(std::string name, bool is_writable,
                       PropertyCreator creator) 
        : _name(name), 
          _creator(creator),
          _is_writable(is_writable),
          _type(type_string_traits::type_to_string((S*)(nullptr)))
    {
    }

    const std::string& get_type() const override
    {
        return _type; 
    }
    const std::string& get_name() const override
    {
        return _name;
    }
   
    std::unique_ptr<IProperty> create(
        std::weak_ptr<INotifyPropertyChanged> owner) const override
    {
        return std::unique_ptr<IProperty>(_creator(owner, get_name()));
    }
    
    bool is_writable() const override
    {
        return _is_writable;
    }
    
private:
    std::string _name;
    std::string _type;
    bool _is_writable;
    PropertyCreator _creator;
};

class IMultitype : public IVirtualBase
{
public:
    virtual void set_value(int index, const std::string& str) = 0;
    virtual std::string get_value(int index) const = 0;
    virtual ICopyable* get(int index) = 0;
};

class ITypeConverter : public IVirtualBase
{
public:
    virtual const std::string& get_from() const = 0;
    virtual const std::string& get_to() const = 0;
    virtual void apply(IMultitype& var, bool forward) const = 0;
    
    virtual std::unique_ptr<IMultitype> make_state() const = 0;
};

template<class T>
class InlineVariable : public PropertyBase<T>
{
public:
    InlineVariable() 
        : PropertyBase<T>(std::shared_ptr<INotifyPropertyChanged>(nullptr), ""), 
          _val()
    {}
    
    bool is_writable() const { return true; }
    
    void set(T val) override { _val = val; }
    T get() const override { return _val; }
private:
    T _val;
};

template<class T, class S>
class DualVariable : public IMultitype
{
public:
    DualVariable() 
        : _from(), _to() 
    {}

    void set_value(int index, const std::string& val) override
    {
        if (index == 0) _from.set_value(val);
        else if (index == 1) _to.set_value(val);
        else throw std::runtime_error(str() << "Invalid index" << index);
    }
    std::string get_value(int index) const override
    {
        if (index == 0) return _from.get_value();
        else if (index == 1) return _to.get_value();
        else throw std::runtime_error(str() << "Invalid index" << index);
    }
    ICopyable* get(int index) override
    {
        if (index == 0) return &_from;
        else if (index == 1) return &_to;
        else throw std::runtime_error(str() << "Invalid index" << index);
    }

    InlineVariable<T>& get_from() { return _from; }
    InlineVariable<S>& get_to() { return _to; }
    const InlineVariable<T>& get_from() const { return _from; }
    const InlineVariable<S>& get_to() const { return _to; }
    
private:
    InlineVariable<T> _from;
    InlineVariable<S> _to;
};

template<class T, class S>
class TypeConverterBase : public ITypeConverter
{
public:
    TypeConverterBase()
        : _from(type_string_traits::type_to_string((T*)(nullptr))),
          _to(type_string_traits::type_to_string((S*)(nullptr)))
    {
    }

    virtual S convert(T) const = 0;
    virtual T convert(S) const = 0;
    
    const std::string& get_from() const override { return _from; }
    const std::string& get_to() const override { return _to; }
    
    void apply(IMultitype& var, bool forward) const override
    {
        if (forward)
        {                                         
            auto dual = static_cast<DualVariable<T, S>*>(&var);
            if (dual)
            {
                auto t = dual->get_from().get();
                auto s = convert(t);
                dual->get_to().set(s);
            }
            else
            {
                auto t = type_string_traits::parse(var.get_value(0), (T*)(nullptr));
                auto s = convert(t);
                var.set_value(1, type_string_traits::to_string(s));
            }
        }
        else
        {                                         
            auto dual = static_cast<DualVariable<T,S>*>(&var);
            if (dual)
            {
                auto s = dual->get_to().get();
                auto t = convert(s);
                dual->get_from().set(t);
            }
            else
            {
                auto s = type_string_traits::parse(var.get_value(1), (S*)(nullptr));
                auto t = convert(s);
                var.set_value(0, type_string_traits::to_string(t));
            }
        }
    }
    
    std::unique_ptr<IMultitype> make_state() const override
    {
        return std::unique_ptr<IMultitype>(new DualVariable<T, S>());
    }
    
private:
    std::string _from;
    std::string _to;
};

template<class T, class S>
class CastConverter : public TypeConverterBase<T, S>
{
public:
    S convert(T x) const override { return static_cast<S>(x); }
    T convert(S x) const override { return static_cast<S>(x); }
};

template<class T, class S>
class FieldProperty : public PropertyBase<S>
{
public:
    FieldProperty(std::weak_ptr<INotifyPropertyChanged> owner, 
                  std::string name, S T::* field) 
        : PropertyBase<S>(owner, name), _field(field)
    {
        auto s = owner.lock();
        _t = static_cast<T*>(s.get());
    }
    
    void set(S val) override
    {
        (*_t).*_field = val;
    }
    
    S get() const override
    {
        return (*_t).*_field;
    }
    
private:
    T* _t;
    S T::* _field;
};

template<class T, class S>
class ReadOnlyProperty : public PropertyBase<S>
{
public:
    typedef S (T::*TGetter)() const;

    ReadOnlyProperty(std::weak_ptr<INotifyPropertyChanged> owner, 
                     std::string name, TGetter getter) 
        : PropertyBase<S>(owner, name), _getter(getter)
    {
        auto s = owner.lock();
        _t = static_cast<T*>(s.get());
    }
    
    void set(S val) override
    {
        throw std::runtime_error(str() << "Property is read-only!");
    }
    
    S get() const override
    {
        return ((*_t).*_getter)();
    }

private:
    T* _t;
    TGetter _getter;
};

template<class T, class S>
class ReadWriteProperty : public PropertyBase<S>
{
public:
    typedef S (T::*TGetter)() const;
    typedef void (T::*TSetter)(S);

    ReadWriteProperty(std::weak_ptr<INotifyPropertyChanged> owner, 
                      std::string name, TGetter getter, TSetter setter) 
        : PropertyBase<S>(owner, name), _getter(getter), _setter(setter)
    {
        auto s = owner.lock();
        _t = static_cast<T*>(s.get());
    }
    
    void set(S val) override
    {
        ((*_t).*_setter)(val);
    }
    
    S get() const override
    {
        return ((*_t).*_getter)();
    }

private:
    T* _t;
    TGetter _getter;
    TSetter _setter;
};

template<class T, class S>
class ReadWritePropertyLambda : public PropertyBase<S>
{
public:
    typedef std::function<S(std::weak_ptr<INotifyPropertyChanged>)> TGetter;
    typedef std::function<void(std::weak_ptr<INotifyPropertyChanged>,S)> TSetter;

    ReadWritePropertyLambda(std::weak_ptr<INotifyPropertyChanged> owner, 
                            std::string name, TGetter getter, TSetter setter) 
        : PropertyBase<S>(owner, name), _owner(owner),
          _getter(getter), _setter(setter)
    {
    }
    
    void set(S val) override
    {
        _setter(_owner, val);
    }
    
    S get() const override
    {
        return _getter(_owner);
    }

private:
    TGetter _getter;
    TSetter _setter;
    std::weak_ptr<INotifyPropertyChanged> _owner;
};


#define DefineClass(T) using __Type = T;\
    return std::make_shared<TypeDefinitionBuilder<__Type>>(#T) 
    
#define AddField(F) add(&__Type::F, #F)

#define AddProperty(R, W) add(&__Type::R, &__Type::W, #R, #W)

#define ExtendClass(T, S) using __Type = T;\
    auto base_ptr = TypeDefinition<S>::make();\
    auto base_builder = dynamic_cast<TypeDefinitionBuilder<S>*>(base_ptr.get());\
    return base_builder->extend<T>(#T)


template<class T, bool is_constructable>
struct Ctor
{
    static T* construct(const std::string& name, T* = nullptr); 
};

template<class T>
struct Ctor<T, true>
{
    static T* construct(const std::string& name, T* = nullptr)
    {
        return new T();
    }
};

template<class T>
struct Ctor<T, false>
{
    static T* construct(const std::string& name, T* = nullptr)
    {
        throw std::runtime_error(str() << 
            "Type " << name << " is not default constructible!");
    }
};

template<class T>
class TypeDefinitionBuilder : public ITypeDefinition
{
public:
    TypeDefinitionBuilder(std::string name) 
        : _name(name) {}
        
    std::unique_ptr<INotifyPropertyChanged> default_construct() const override
    {
        return std::unique_ptr<INotifyPropertyChanged>(
            Ctor<T, std::is_default_constructible<T>::value>::construct(get_type()));
    }
        
    const std::string& get_type() const override
    {
        return _name;
    }
    
    const IPropertyDefinition* get_property(const std::string& name) const 
    {
        auto it = _properties.find(name);
        if (it != _properties.end())
        {
            return it->second.get();
        }
        throw std::runtime_error(str() << "Property " << name << " not found!");
    }
    IPropertyDefinition* get_property(const std::string& name)
    {
        return _properties[name].get();
    }
    const std::vector<std::string> get_property_names() const
    {
        return _property_names;
    }
    
    void assign_fields(std::vector<std::string> property_names,
                       std::map<std::string, 
                       std::shared_ptr<IPropertyDefinition>> properties)
    {
        _property_names = property_names;
        _properties = properties;
    }
    
    template<class S>
    std::shared_ptr<TypeDefinitionBuilder<S>> extend(const char* name) const
    {
        auto result = std::make_shared<TypeDefinitionBuilder<S>>(name);
        result->assign_fields(_property_names, _properties);
        return result;
    }
    
    template<class S>
    std::shared_ptr<TypeDefinitionBuilder<T>> add(S T::* f, const char* name) const
    {
        auto result = std::make_shared<TypeDefinitionBuilder<T>>(*this);
        std::string name_str(name);
        result->_property_names.push_back(name_str);
        auto ptr = std::make_shared<PropertyDefinition<S>>(name_str, true,
        [f](std::weak_ptr<INotifyPropertyChanged> owner, const std::string& name){
            return new FieldProperty<T, S>(owner, name, f);
        });
        result->_properties[name] = ptr;
        return result;
    }
    
    template<class S>
    std::shared_ptr<TypeDefinitionBuilder<T>> add(S (T::*f)() const, const char* name) const
    {
        auto result = std::make_shared<TypeDefinitionBuilder<T>>(*this);
        auto name_str = remove_prefix("get_", name);
        if (name_str == name) name_str = remove_prefix("is_", name);
        result->_property_names.push_back(name_str);

        auto ptr = std::make_shared<PropertyDefinition<S>>(name_str, false,
        [f](std::weak_ptr<INotifyPropertyChanged> owner, 
            const std::string& name){
            return new ReadOnlyProperty<T, S>(owner, name, f);
        });
        
        result->_properties[name_str] = ptr;
        return result;
    }
    
    template<class S>
    std::shared_ptr<TypeDefinitionBuilder<T>> add(const S& (T::*r)() const, 
                                               void (T::*w)(S), 
                                               const char* rname,
                                               const char* wname) const
    {
        auto result = std::make_shared<TypeDefinitionBuilder<T>>(*this);
        auto name_str = remove_prefix("get_", rname);
        if (name_str == rname) name_str = remove_prefix("is_", rname);
        if (name_str != remove_prefix("set_", wname))
        {
            throw std::runtime_error(str() << "Inconsistent naming for property " 
                    << name_str << "!");
        }
        result->_property_names.push_back(name_str);
        
        auto rf = [r](std::weak_ptr<INotifyPropertyChanged> owner) -> S {
            auto strong = owner.lock();
            auto t = static_cast<T*>(strong.get()); 
            return ((*t).*r)();
        };
        auto wf = [w](std::weak_ptr<INotifyPropertyChanged> owner, S s) {
            auto strong = owner.lock();
            auto t = static_cast<T*>(strong.get()); 
            ((*t).*w)(s);
        };
        
        auto ptr = std::make_shared<PropertyDefinition<S>>(name_str, true,
        [rf, wf](std::weak_ptr<INotifyPropertyChanged> owner, const std::string& name){
            return new ReadWritePropertyLambda<T, S>(owner, name, rf, wf);
        });
        result->_properties[name_str] = ptr;
        return result;
    }
    
    template<class S>
    std::shared_ptr<TypeDefinitionBuilder<T>> add(const S& (T::*r)() const, 
                                               void (T::*w)(const S&), 
                                               const char* rname,
                                               const char* wname) const
    {
        auto result = std::make_shared<TypeDefinitionBuilder<T>>(*this);
        auto name_str = remove_prefix("get_", rname);
        if (name_str == rname) name_str = remove_prefix("is_", rname);
        if (name_str != remove_prefix("set_", wname))
        {
            throw std::runtime_error(str() << "Inconsistent naming for property " 
                    << name_str << "!");
        }
        result->_property_names.push_back(name_str);
        
        auto rf = [r](std::weak_ptr<INotifyPropertyChanged> owner) -> S {
            auto strong = owner.lock();
            auto t = static_cast<T*>(strong.get()); 
            return ((*t).*r)();
        };
        auto wf = [w](std::weak_ptr<INotifyPropertyChanged> owner, S s) {
            auto strong = owner.lock();
            auto t = static_cast<T*>(strong.get()); 
            ((*t).*w)(s);
        };
        
        auto ptr = std::make_shared<PropertyDefinition<S>>(name_str, true,
        [rf, wf](std::weak_ptr<INotifyPropertyChanged> owner, const std::string& name){
            return new ReadWritePropertyLambda<T, S>(owner, name, rf, wf);
        });
        result->_properties[name_str] = ptr;
        return result;
    }
    
    template<class S>
    std::shared_ptr<TypeDefinitionBuilder<T>> add(S (T::*r)() const, 
                                               void (T::*w)(const S&), 
                                               const char* rname,
                                               const char* wname) const
    {
        auto result = std::make_shared<TypeDefinitionBuilder<T>>(*this);
        auto name_str = remove_prefix("get_", rname);
        if (name_str == rname) name_str = remove_prefix("is_", rname);
        if (name_str != remove_prefix("set_", wname))
        {
            throw std::runtime_error(str() << "Inconsistent naming for property " 
                    << name_str << "!");
        }
        result->_property_names.push_back(name_str);
        
        auto rf = [r](std::weak_ptr<INotifyPropertyChanged> owner) -> S {
            auto strong = owner.lock();
            auto t = static_cast<T*>(strong.get()); 
            return ((*t).*r)();
        };
        auto wf = [w](std::weak_ptr<INotifyPropertyChanged> owner, S s) {
            auto strong = owner.lock();
            auto t = static_cast<T*>(strong.get()); 
            ((*t).*w)(s);
        };
        
        auto ptr = std::make_shared<PropertyDefinition<S>>(name_str, true,
        [rf, wf](std::weak_ptr<INotifyPropertyChanged> owner, 
                 const std::string& name){
            return new ReadWritePropertyLambda<T, S>(owner, name, rf, wf);
        });
        result->_properties[name_str] = ptr;
        return result;
    }
    
    template<class S>
    std::shared_ptr<TypeDefinitionBuilder<T>> add(const S& (T::*f)() const, const char* name) const
    {
        auto result = std::make_shared<TypeDefinitionBuilder<T>>(*this);
        auto name_str = remove_prefix("get_", name);
        if (name_str == name) name_str = remove_prefix("is_", name);
        result->_property_names.push_back(name_str);
        
        auto rf = [f](std::weak_ptr<INotifyPropertyChanged> owner) -> S {
            auto strong = owner.lock();
            auto t = static_cast<T*>(strong.get()); 
            return ((*t).*f)();
        };
        auto wf = [name_str](std::weak_ptr<INotifyPropertyChanged>, S s) {
            throw std::runtime_error(str() << "Property " << name_str << " is read-only!");
        };
        
        auto ptr = std::make_shared<PropertyDefinition<S>>(name_str, false,
        [rf, wf](std::weak_ptr<INotifyPropertyChanged> owner, 
                 const std::string& name){
            return new ReadWritePropertyLambda<T, S>(owner, name, rf, wf);
        });
        result->_properties[name_str] = ptr;
        return result;
    }
    
    template<class S>
    std::shared_ptr<TypeDefinitionBuilder<T>> add(S (T::*r)() const, 
                                               void (T::*w)(S), 
                                               const char* rname,
                                               const char* wname) const
    {
        auto result = std::make_shared<TypeDefinitionBuilder<T>>(*this);
        auto name_str = remove_prefix("get_", rname);
        if (name_str == rname) name_str = remove_prefix("is_", rname);
        if (name_str != remove_prefix("set_", wname))
        {
            throw std::runtime_error(str() << "Inconsistent naming for property " 
                    << name_str << "!");
        }
        result->_property_names.push_back(name_str);
        auto ptr = std::make_shared<PropertyDefinition<S>>(name_str, true,
        [r, w](std::weak_ptr<INotifyPropertyChanged> owner, 
               const std::string& name){
            return new ReadWriteProperty<T, S>(owner, name, r, w);
        });
        result->_properties[name_str] = ptr;
        return result;
    }
   
private:
    std::vector<std::string> _property_names;
    std::map<std::string, std::shared_ptr<IPropertyDefinition>> _properties;
    std::string _name;
};

class Binding
{
public:
    void a_to_b();
    void b_to_a();
    
    Binding(TypeFactory& factory,
            std::weak_ptr<INotifyPropertyChanged> a, std::string a_prop,
            std::weak_ptr<INotifyPropertyChanged> b, std::string b_prop,
            std::unique_ptr<ITypeConverter> converter = nullptr);
            
    ~Binding();
    
private:
    bool _is_direct;
    ITypeDefinition* _a_dc;
    ITypeDefinition* _b_dc;
    std::unique_ptr<IProperty> _a_prop_ptr;
    std::unique_ptr<IProperty> _b_prop_ptr;
    IPropertyDefinition* _a_prop_def;
    IPropertyDefinition* _b_prop_def;
    ICopyable* _direct_a;
    ICopyable* _direct_b;
    std::weak_ptr<INotifyPropertyChanged> _a;
    std::weak_ptr<INotifyPropertyChanged> _b;
    std::string _a_prop;
    std::string _b_prop;
    bool _skip_a = false;
    bool _skip_b = false;
    
    std::unique_ptr<ITypeConverter> _converter;
    std::unique_ptr<IMultitype> _converter_state;
    bool _converter_direction;
    TypeFactory& _factory;
};

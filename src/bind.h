#pragma once

#include <functional>
#include <exception>
#include <memory>
#include <string>

#include "parser.h"

class IProperty;

typedef std::function<void(IProperty* prop, 
                           const std::string& old_value,
                           const std::string& new_value)> 
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
    virtual const std::string& get_type() const = 0;
    virtual const std::string& get_name() const = 0;
    
    virtual void set_on_change(OnPropertyChangeCallback on_change) = 0;
};

class IDataContext : public IVirtualBase
{
public:
    virtual const std::string& get_type() const = 0;

    virtual const IProperty* get_property(const std::string& name) const = 0;
    virtual IProperty* get_property(const std::string& name) = 0;
    virtual const std::vector<std::string> get_property_names() const = 0;
};

class IBindableObject : public IVirtualBase
{
public:
    virtual void set_on_change(OnFieldChangeCallback on_change) = 0;
    virtual std::shared_ptr<IDataContext> make_data_context() = 0;
};

class BindableObjectBase : public IBindableObject
{
public:
    BindableObjectBase() : _on_change([](const char*){}) {}

    void set_on_change(OnFieldChangeCallback on_change) override
    {
        _on_change = on_change;
    }
    
    void fire_property_change(const char* property_name)
    {
        _on_change(property_name);
    }
    
private:
    OnFieldChangeCallback _on_change;
};

#define DefineClass(T) using __Type = T;\
    return std::make_shared<DataContextBuilder<__Type>>(this, #T) 
    
#define AddField(F) add(&__Type::F, #F)

template<class T, class S>
class FieldProperty : public IProperty
{
public:

    FieldProperty(IBindableObject* owner, S T::* field, std::string name) 
        : _owner(owner), _on_change([](IProperty*, 
                                       const std::string&,
                                       const std::string&){}),
          _field(field), _name(name), _type("unknown")// TODO: fix
    {}
    
    void set_value(std::string value) override
    {
        S s;
        type_string_traits::parse(value, &s);
        auto old_value = get_value();
        auto t = (T*)_owner;
        (*t).*_field = s;
        _on_change(this, old_value, value);
    }
    std::string get_value() const override
    {
        auto t = (T*)_owner;
        return type_string_traits::to_string((*t).*_field);
    }
    const std::string& get_type() const override
    {
        return _type; 
    }
    const std::string& get_name() const override
    {
        return _name;
    }
    
    void set_on_change(OnPropertyChangeCallback on_change) override
    {
        _on_change = on_change;
    }
    
private:
    OnPropertyChangeCallback _on_change;
    IBindableObject* _owner;
    S T::* _field;
    std::string _name;
    std::string _type;
};

template<class T>
class DataContextBuilder : public IDataContext
{
public:
    DataContextBuilder(IBindableObject* owner, std::string name) 
        : _owner(owner), _name(name) {}
        
    const std::string& get_type() const override
    {
        return _name;
    }
    
    const IProperty* get_property(const std::string& name) const 
    {
        auto it = _properties.find(name);
        if (it != _properties.end())
        {
            return it->second.get();
        }
        throw std::runtime_error("Property not found!");
    }
    IProperty* get_property(const std::string& name)
    {
        return _properties[name].get();
    }
    const std::vector<std::string> get_property_names() const
    {
        return _property_names;
    }
    
    template<class S>
    std::shared_ptr<DataContextBuilder<T>> add(S T::* f, const char* name) const
    {
        auto result = std::make_shared<DataContextBuilder<T>>(*this);
        std::string name_str(name);
        result->_property_names.push_back(name_str);
        auto ptr = std::make_shared<FieldProperty<T, S>>(_owner, f, name_str);
        result->_properties[name] = ptr;
        return result;
    }
private:
    mutable IBindableObject* _owner;
    std::vector<std::string> _property_names;
    std::map<std::string, std::shared_ptr<IProperty>> _properties;
    std::string _name;
};


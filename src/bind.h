#pragma once

#include <functional>
#include <exception>
#include <memory>
#include <string>
#include <map>

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
    
    virtual void subscribe_on_change(void* owner, 
                                     OnPropertyChangeCallback on_change) = 0;
    virtual void unsubscribe_on_change(void* owner) = 0;
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
    virtual void subscribe_on_change(void* owner, 
                                     OnFieldChangeCallback on_change) = 0;
    virtual void unsubscribe_on_change(void* owner) = 0;
    
    virtual std::shared_ptr<IDataContext> make_data_context() = 0;
};

class BindableObjectBase : public IBindableObject
{
public:
    BindableObjectBase() : _on_change() {}
    
    void fire_property_change(const char* property_name)
    {
        for(auto& evnt : _on_change)
        {
            evnt.second(property_name);
        }
    }
    
    void subscribe_on_change(void* owner, 
                             OnFieldChangeCallback on_change) override
    {
        _on_change[owner] = on_change;
    }
    void unsubscribe_on_change(void* owner) override
    {
        _on_change.erase(owner);
    }
    
private:
    std::map<void*,OnFieldChangeCallback> _on_change;
};

#define DefineClass(T) using __Type = T;\
    return std::make_shared<DataContextBuilder<__Type>>(this, #T) 
    
#define AddField(F) add(&__Type::F, #F)

template<class T, class S>
class PropertyBase : public IProperty
{
public:

    PropertyBase(IBindableObject* owner, std::string name) 
        : _owner(owner), _on_change(),
          _name(name), 
          _type(type_string_traits::type_to_string((S*)(nullptr)))
    {
        _owner->subscribe_on_change(this,[this](const char* field){
            if (field == _name)
            {
                std::string value = get_value();
                for(auto& evnt : _on_change)
                {
                    evnt.second(this, _value, value);
                }
                _value = value;
            }
        });
        _value = get_value();
    }
    
    ~PropertyBase()
    {
        _owner->unsubscribe_on_change(this);
    }
    
    virtual void set(S val) = 0;
    virtual S get() const = 0;
    
    void set_value(std::string value) override
    {
        auto s = type_string_traits::parse(value, (S*)(nullptr));
        auto old_value = get_value();
        set(s);
        _value = value;
        for(auto& evnt : _on_change)
        {
            evnt.second(this, old_value, value);
        }
    }
    std::string get_value() const override
    {
        return type_string_traits::to_string(get());
    }
    const std::string& get_type() const override
    {
        return _type; 
    }
    const std::string& get_name() const override
    {
        return _name;
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
    IBindableObject* _owner;
    std::string _name;
    std::string _type;
    mutable std::string _value;
};

template<class T, class S>
class FieldProperty : public PropertyBase<T, S>
{
public:
    FieldProperty(IBindableObject* owner, std::string name, S T::* field) 
        : PropertyBase<T,S>(owner, name), _field(field), _t((T*)owner)
    {
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
        auto ptr = std::make_shared<FieldProperty<T, S>>(_owner, name_str, f);
        result->_properties[name] = ptr;
        return result;
    }
private:
    mutable IBindableObject* _owner;
    std::vector<std::string> _property_names;
    std::map<std::string, std::shared_ptr<IProperty>> _properties;
    std::string _name;
};


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
    virtual IDataContext* fetch_self() = 0;
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
    
    IDataContext* fetch_self() override
    {
        if (!_self) _self = make_data_context();
        return _self.get();
    }
    
private:
    std::map<void*,OnFieldChangeCallback> _on_change;
    mutable std::shared_ptr<IDataContext> _self = nullptr;
};

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
    }
    
    ~PropertyBase()
    {
        _owner->unsubscribe_on_change(this);
    }
    
    void init()
    {
        _value = get_value();
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
        : PropertyBase<T,S>(owner, name), _field(field), 
          _t(dynamic_cast<T*>(owner))
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

template<class T, class S>
class ReadOnlyProperty : public PropertyBase<T, S>
{
public:
    typedef S (T::*TGetter)() const;

    ReadOnlyProperty(IBindableObject* owner, std::string name, 
                     TGetter getter) 
        : PropertyBase<T,S>(owner, name), _getter(getter), 
          _t(dynamic_cast<T*>(owner))
    {
    }
    
    void set(S val) override
    {
        throw std::runtime_error("Property is read-only!");
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
class ReadWriteProperty : public PropertyBase<T, S>
{
public:
    typedef S (T::*TGetter)() const;
    typedef void (T::*TSetter)(S);

    ReadWriteProperty(IBindableObject* owner, std::string name, 
                      TGetter getter, TSetter setter) 
        : PropertyBase<T,S>(owner, name), _getter(getter), _setter(setter),
          _t(dynamic_cast<T*>(owner))
    {
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
class ReadWritePropertyLambda : public PropertyBase<T, S>
{
public:
    typedef std::function<S()> TGetter;
    typedef std::function<void(S)> TSetter;

    ReadWritePropertyLambda(IBindableObject* owner, std::string name, 
                      TGetter getter, TSetter setter) 
        : PropertyBase<T,S>(owner, name), _getter(getter), _setter(setter)
    {
    }
    
    void set(S val) override
    {
        _setter(val);
    }
    
    S get() const override
    {
        return _getter();
    }

private:
    TGetter _getter;
    TSetter _setter;
};


#define DefineClass(T) using __Type = T;\
    return std::make_shared<DataContextBuilder<__Type>>(this, #T) 
    
#define AddField(F) add(&__Type::F, #F)

#define AddProperty(R, W) add(&__Type::R, &__Type::W, #R, #W)

#define ExtendClass(T, S) using __Type = T;\
    auto base_ptr = S::make_data_context();\
    auto base_builder = dynamic_cast<DataContextBuilder<S>*>(base_ptr.get());\
    return base_builder->extend(#T, this)

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
    
    void assign_fields(std::vector<std::string> property_names,
                       std::map<std::string, std::shared_ptr<IProperty>> properties)
    {
        _property_names = property_names;
        _properties = properties;
    }
    
    template<class S>
    std::shared_ptr<DataContextBuilder<S>> extend(const char* name, S* owner) const
    {
        auto result = std::make_shared<DataContextBuilder<S>>(owner, name);
        result->assign_fields(_property_names, _properties);
        return result;
    }
    
    template<class S>
    std::shared_ptr<DataContextBuilder<T>> add(S T::* f, const char* name) const
    {
        auto result = std::make_shared<DataContextBuilder<T>>(*this);
        std::string name_str(name);
        result->_property_names.push_back(name_str);
        auto ptr = std::make_shared<FieldProperty<T, S>>(_owner, name_str, f);
        ptr->init();
        result->_properties[name] = ptr;
        return result;
    }
    
    template<class S>
    std::shared_ptr<DataContextBuilder<T>> add(S (T::*f)() const, const char* name) const
    {
        auto result = std::make_shared<DataContextBuilder<T>>(*this);
        auto name_str = remove_prefix("get_", name);
        if (name_str == name) name_str = remove_prefix("is_", name);
        result->_property_names.push_back(name_str);
        auto ptr = std::make_shared<ReadOnlyProperty<T, S>>(_owner, name_str, f);
        ptr->init();
        result->_properties[name_str] = ptr;
        return result;
    }
    
    template<class S>
    std::shared_ptr<DataContextBuilder<T>> add(const S& (T::*r)() const, 
                                               void (T::*w)(S), 
                                               const char* rname,
                                               const char* wname) const
    {
        auto result = std::make_shared<DataContextBuilder<T>>(*this);
        auto name_str = remove_prefix("get_", rname);
        if (name_str == rname) name_str = remove_prefix("is_", rname);
        if (name_str != remove_prefix("set_", wname))
        {
            throw std::runtime_error("Inconsistent property getter/setter naming!");
        }
        result->_property_names.push_back(name_str);
        auto t = dynamic_cast<T*>(_owner); 
        auto rf = [t, r]() -> S {
            return ((*t).*r)();
        };
        auto wf = [t, w](S s) {
            ((*t).*w)(s);
        };
        
        auto ptr = std::make_shared<ReadWritePropertyLambda<T, S>>(_owner, name_str, rf, wf);
        ptr->init();
        result->_properties[name_str] = ptr;
        return result;
    }
    
    template<class S>
    std::shared_ptr<DataContextBuilder<T>> add(const S& (T::*f)() const, const char* name) const
    {
        auto result = std::make_shared<DataContextBuilder<T>>(*this);
        auto name_str = remove_prefix("get_", name);
        if (name_str == name) name_str = remove_prefix("is_", name);
        result->_property_names.push_back(name_str);
        auto t = dynamic_cast<T*>(_owner); 
        auto rf = [t, f]() -> S {
            return ((*t).*f)();
        };
        auto wf = [](S s) {
            throw std::runtime_error("Property is read-only!");
        };
        
        auto ptr = std::make_shared<ReadWritePropertyLambda<T, S>>(_owner, name_str, rf, wf);
        ptr->init();
        result->_properties[name_str] = ptr;
        return result;
    }
    
    template<class S>
    std::shared_ptr<DataContextBuilder<T>> add(S (T::*r)() const, 
                                               void (T::*w)(S), 
                                               const char* rname,
                                               const char* wname) const
    {
        auto result = std::make_shared<DataContextBuilder<T>>(*this);
        auto name_str = remove_prefix("get_", rname);
        if (name_str == rname) name_str = remove_prefix("is_", rname);
        if (name_str != remove_prefix("set_", wname))
        {
            throw std::runtime_error("Inconsistent property getter/setter naming!");
        }
        result->_property_names.push_back(name_str);
        auto ptr = std::make_shared<ReadWriteProperty<T, S>>(_owner, name_str, r, w);
        ptr->init();
        result->_properties[name_str] = ptr;
        return result;
    }
private:
    mutable IBindableObject* _owner;
    std::vector<std::string> _property_names;
    std::map<std::string, std::shared_ptr<IProperty>> _properties;
    std::string _name;
};

class Binding
{
public:
    Binding(IBindableObject* a, std::string a_prop,
            IBindableObject* b, std::string b_prop)
    {
        _a_dc = a->fetch_self();
        _b_dc = b->fetch_self();
        _a = a; _b = b;
        _a_prop = a_prop;
        _b_prop = b_prop;
        
        _a_prop_ptr = _a_dc->get_property(a_prop);
        if (!_a_prop_ptr) throw std::runtime_error("Property not found!");
        
        _b_prop_ptr = _b_dc->get_property(b_prop);
        if (!_b_prop_ptr) throw std::runtime_error("Property not found!");
        
        _a_prop_ptr->subscribe_on_change(this, [=](IProperty* sender, 
                                                  const std::string& old_value,
                                                  const std::string& new_value)
        {
            if (!_skip_a)
            {
                _skip_b = true;
                _b_prop_ptr->set_value(new_value);
                _skip_b = false;
            }
        });
        
        _b_prop_ptr->subscribe_on_change(this, [=](IProperty* sender, 
                                                  const std::string& old_value,
                                                  const std::string& new_value)
        {
            if (!_skip_b)
            {
                _skip_a = true;
                _a_prop_ptr->set_value(new_value);
                _skip_a = false;
            }
        });
    }
    
    ~Binding()
    {
        _a_prop_ptr->unsubscribe_on_change(this);
        _b_prop_ptr->unsubscribe_on_change(this);
    }
    
private:
    IDataContext* _a_dc;
    IDataContext* _b_dc;
    IProperty* _a_prop_ptr;
    IProperty* _b_prop_ptr;
    IBindableObject* _a;
    IBindableObject* _b;
    std::string _a_prop;
    std::string _b_prop;
    bool _skip_a = false;
    bool _skip_b = false;
};

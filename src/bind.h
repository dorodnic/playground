#pragma once

#include <functional>
#include <exception>
#include <memory>
#include <string>
#include <map>

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
    virtual const std::string& get_type() const = 0;
    virtual const std::string& get_name() const = 0;
    
    virtual bool is_writable() const = 0;
    
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
    BindableObjectBase();
    
    void fire_property_change(const char* property_name);
    void subscribe_on_change(void* owner, 
                             OnFieldChangeCallback on_change) override;
    void unsubscribe_on_change(void* owner) override;
    IDataContext* fetch_self() override;
    
private:
    std::map<void*,OnFieldChangeCallback> _on_change;
    mutable std::shared_ptr<IDataContext> _self = nullptr;
};

class ICopyable : public virtual IVirtualBase
{
public:
    virtual void copy_to(ICopyable* to) = 0;
    virtual void copy_from(ICopyable* from) = 0;
};

template<class S>
class PropertyBase : public IProperty, public ICopyable
{
public:

    PropertyBase(IBindableObject* owner, std::string name) 
        : _owner(owner), _on_change(),
          _name(name), 
          _type(type_string_traits::type_to_string((S*)(nullptr)))
    {
        if (_owner)
        {
            _owner->subscribe_on_change(this,[this](const char* field){
                if (field == _name)
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
        if (_owner) _owner->unsubscribe_on_change(this);
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
};

class IMultitype : public virtual IVirtualBase
{
public:
    virtual const std::string& get_type(int index) const = 0;
    virtual void set_value(int index, const std::string& str) = 0;
    virtual std::string get_value(int index) const = 0;
    virtual ICopyable* get(int index) = 0;
};

class ITypeConverter : public virtual IVirtualBase
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
        : PropertyBase<T>(nullptr, ""), 
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

    const std::string& get_type(int index) const override
    {
        if (index == 0) return _from.get_type();
        else if (index == 1) return _to.get_type();
        else throw std::runtime_error(str() << "Invalid index" << index);
    }
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
            if (var.get_type(0) != get_from()) 
                throw std::runtime_error(str() << "Converter expected input type "
                                         << get_from() << " but received " 
                                         << var.get_type(0) << "!");
            if (var.get_type(1) != get_to()) 
                throw std::runtime_error(str() << "Converter expected output type "
                                         << get_to() << " but received " 
                                         << var.get_type(1) << "!");
                                         
            auto dual = dynamic_cast<DualVariable<T, S>*>(&var);
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
            if (var.get_type(0) != get_from()) 
                throw std::runtime_error(str() << "Converter expected input type "
                                         << get_from() << " but received " 
                                         << var.get_type(0) << "!");
            if (var.get_type(1) != get_to()) 
                throw std::runtime_error(str() << "Converter expected output type "
                                         << get_to() << " but received " 
                                         << var.get_type(1) << "!");
                                         
            auto dual = dynamic_cast<DualVariable<T,S>*>(&var);
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
    FieldProperty(IBindableObject* owner, std::string name, S T::* field) 
        : PropertyBase<S>(owner, name), _field(field), 
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
    
    bool is_writable() const override
    {
        return true;
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

    ReadOnlyProperty(IBindableObject* owner, std::string name, 
                     TGetter getter) 
        : PropertyBase<S>(owner, name), _getter(getter), 
          _t(dynamic_cast<T*>(owner))
    {
    }
    
    
    bool is_writable() const override
    {
        return false;
    }
    
    void set(S val) override
    {
        throw std::runtime_error(str() << "Property " << this->get_name() << " is read-only!");
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

    ReadWriteProperty(IBindableObject* owner, std::string name, 
                      TGetter getter, TSetter setter) 
        : PropertyBase<S>(owner, name), _getter(getter), _setter(setter),
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
    
    
    bool is_writable() const override
    {
        return true;
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
    typedef std::function<S()> TGetter;
    typedef std::function<void(S)> TSetter;

    ReadWritePropertyLambda(IBindableObject* owner, std::string name, 
                      TGetter getter, TSetter setter, bool writable) 
        : PropertyBase<S>(owner, name), 
          _getter(getter), _setter(setter), _is_writable(writable)
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

    bool is_writable() const override { return true; }

private:
    TGetter _getter;
    TSetter _setter;
    bool _is_writable;
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
        throw std::runtime_error(str() << "Property " << name << " not found!");
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
            throw std::runtime_error(str() << "Inconsistent naming for property " 
                    << name_str << "!");
        }
        result->_property_names.push_back(name_str);
        auto t = dynamic_cast<T*>(_owner); 
        auto rf = [t, r]() -> S {
            return ((*t).*r)();
        };
        auto wf = [t, w](S s) {
            ((*t).*w)(s);
        };
        
        auto ptr = std::make_shared<ReadWritePropertyLambda<T, S>>
                        (_owner, name_str, rf, wf, true);
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
        auto wf = [name_str](S s) {
            throw std::runtime_error(str() << "Property " << name_str << " is read-only!");
        };
        
        auto ptr = std::make_shared<ReadWritePropertyLambda<T, S>>
            (_owner, name_str, rf, wf, false);
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
            throw std::runtime_error(str() << "Inconsistent naming for property " 
                    << name_str << "!");
        }
        result->_property_names.push_back(name_str);
        auto ptr = std::make_shared<ReadWriteProperty<T, S>>(_owner, name_str, r, w);
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
    void a_to_b();
    void b_to_a();
    
    Binding(IBindableObject* a, std::string a_prop,
            IBindableObject* b, std::string b_prop,
            std::unique_ptr<ITypeConverter> converter = nullptr);
    
    static std::unique_ptr<Binding> bind(
            IBindableObject* a, std::string a_prop,
            IBindableObject* b, std::string b_prop,
            std::unique_ptr<ITypeConverter> converter = nullptr);
            
    ~Binding();
    
private:
    bool _is_direct;
    IDataContext* _a_dc;
    IDataContext* _b_dc;
    IProperty* _a_prop_ptr;
    IProperty* _b_prop_ptr;
    ICopyable* _direct_a;
    ICopyable* _direct_b;
    IBindableObject* _a;
    IBindableObject* _b;
    std::string _a_prop;
    std::string _b_prop;
    bool _skip_a = false;
    bool _skip_b = false;
    
    std::unique_ptr<ITypeConverter> _converter;
    std::unique_ptr<IMultitype> _converter_state;
    bool _converter_direction;
};

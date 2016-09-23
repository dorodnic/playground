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
        auto p = dynamic_cast<PropertyBase<S>*>(to);
        p->set(get((S*)nullptr));
    }
    void copy_from(ICopyable* from) override
    {
        auto p = dynamic_cast<PropertyBase<S>*>(from);
        set(p->get((S*)nullptr));
    }
    
    virtual void set(S val) = 0;
    virtual S get(S* token) const = 0;
    
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
        return type_string_traits::to_string(get((S*)nullptr));
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
};

class ITypeConverter : public virtual IVirtualBase
{
public:
    virtual const std::string& get_from() const = 0;
    virtual const std::string& get_to() const = 0;
    virtual void apply(IMultitype& var, bool forward) const = 0;
    
    virtual std::unique_ptr<IMultitype> make_state() const = 0;
};

template<class T, class S>
class DualVariable : public IMultitype, 
                     public PropertyBase<T>, 
                     public PropertyBase<S>
{
public:
    DualVariable() 
        : PropertyBase<T>(nullptr, ""), 
          PropertyBase<S>(nullptr, ""), 
          _from(), _to() 
    {}

    const std::string& get_type(int index) const override
    {
        if (index == 0) return PropertyBase<T>::get_type();
        else if (index == 1) return PropertyBase<S>::get_type();
        else throw std::runtime_error(str() << "Invalid index" << index);
    }
    void set_value(int index, const std::string& val) override
    {
        if (index == 0) PropertyBase<T>::set_value(val);
        else if (index == 1) PropertyBase<S>::set_value(val);
        else throw std::runtime_error(str() << "Invalid index" << index);
    }
    std::string get_value(int index) const override
    {
        if (index == 0) return PropertyBase<T>::get_value();
        else if (index == 1) return PropertyBase<S>::get_value();
        else throw std::runtime_error(str() << "Invalid index" << index);
    }
    
    bool is_writable() const { return true; }
    
    void set(S val) override { _to = val; }
    S get(S*) const override { return _to; }
    void set(T val) override { _from = val; }
    T get(T*) const override { return _from; }
    
private:
    T _from;
    S _to;
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
                auto t = dual->get((T*)nullptr);
                auto s = convert(t);
                dual->set(s);
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
                auto s = dual->get((S*)nullptr);
                auto t = convert(s);
                dual->set(t);
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
    
    S get(S*) const override
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
    
    S get(S*) const override
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
    
    S get(S*) const override
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
    
    S get(S*) const override
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
    void a_to_b()
    {
        if (_is_direct)
        {
            _direct_b->copy_from(_direct_a);
        }
        else
        {
            if (_converter)
            {
                if (_direct_converter_state)
                {
                    _direct_b->copy_to(_direct_converter_state);
                    _converter->apply(*_converter_state, _converter_direction);
                    _direct_a->copy_from(_direct_converter_state);
                }
                else
                {
                    _converter_state->set_value(1, _a_prop_ptr->get_value());
                    _converter->apply(*_converter_state, _converter_direction);
                    _b_prop_ptr->set_value(_converter_state->get_value(0));
                }
            }
            else
            {
                _b_prop_ptr->set_value(_a_prop_ptr->get_value());
            }
        }
    }
    
    void b_to_a()
    {
        if (_is_direct)
        {
            _direct_a->copy_from(_direct_b);
        }
        else
        {
            if (_converter)
            {
                if (_direct_converter_state)
                {
                    _direct_a->copy_to(_direct_converter_state);
                    _converter->apply(*_converter_state, !_converter_direction);
                    _direct_b->copy_from(_direct_converter_state);
                }
                else
                {
                    _converter_state->set_value(0, _b_prop_ptr->get_value());
                    _converter->apply(*_converter_state, !_converter_direction);
                    _a_prop_ptr->set_value(_converter_state->get_value(1));
                }
            }
            else
            {
                _a_prop_ptr->set_value(_b_prop_ptr->get_value());
            }
        }
    }

    Binding(IBindableObject* a, std::string a_prop,
            IBindableObject* b, std::string b_prop,
            std::unique_ptr<ITypeConverter> converter = nullptr)
    {
        _a_dc = a->fetch_self();
        _b_dc = b->fetch_self();
        _a = a; _b = b;
        _a_prop = a_prop;
        _b_prop = b_prop;
        
        _converter = std::move(converter);
        if (_converter)
        {
            _converter_state = _converter->make_state();
        }
        
        _a_prop_ptr = _a_dc->get_property(a_prop);
        if (!_a_prop_ptr) 
            throw std::runtime_error(str() << "Property " << a_prop << " not found!");
        
        _b_prop_ptr = _b_dc->get_property(b_prop);
        if (!_b_prop_ptr) 
            throw std::runtime_error(str() << "Property " << b_prop << " not found!");
        
        _is_direct = _a_prop_ptr->get_type() == _b_prop_ptr->get_type();
        if (_is_direct)
        {
            _direct_a = dynamic_cast<ICopyable*>(_a_prop_ptr);
            _direct_b = dynamic_cast<ICopyable*>(_b_prop_ptr);
        }
        
        if (_converter)
        {
            if ((_b_prop_ptr->get_type() != _converter->get_from()) &&
                (_a_prop_ptr->get_type() != _converter->get_from()))
            {
                throw std::runtime_error(str() << "Binding converter must be from either "
                                         << _b_prop_ptr->get_type() << " or "
                                         << _a_prop_ptr->get_type());
            }
            if ((_a_prop_ptr->get_type() != _converter->get_to()) &&
                (_b_prop_ptr->get_type() != _converter->get_to()))
            {
                throw std::runtime_error(str() << "Binding converter must be to either "
                                         << _b_prop_ptr->get_type() << " or "
                                         << _a_prop_ptr->get_type());
            }
            if (_is_direct)
            {
                throw std::runtime_error(
                        str() << "Can't use a converter with binding of same type!");
            }
            _direct_converter_state = dynamic_cast<ICopyable*>(_converter_state.get());
            
            _converter_direction = _a_prop_ptr->get_type() != _converter->get_to();
        }
        
        if (_b_prop_ptr->is_writable())
        {
            _a_prop_ptr->subscribe_on_change(this, [=](IProperty* sender)
            {
                if (!_skip_a)
                {
                    _skip_b = true;
                    a_to_b();
                    _skip_b = false;
                }
            });
        }
        
        if (_a_prop_ptr->is_writable())
        {
            _b_prop_ptr->subscribe_on_change(this, [=](IProperty* sender)
            {
                if (!_skip_b)
                {
                    _skip_a = true;
                    b_to_a();
                    _skip_a = false;
                }
            });
        }
        
        if (_a_prop_ptr->is_writable())
        {
            b_to_a();
        }
        else if (_b_prop_ptr->is_writable())
        {
            a_to_b();
        }
        else
        {
            throw std::runtime_error("Both properties under binding can't be read-only!");
        }
    }
    
    static std::unique_ptr<Binding> bind(
            IBindableObject* a, std::string a_prop,
            IBindableObject* b, std::string b_prop,
            std::unique_ptr<ITypeConverter> converter = nullptr)
    {
        std::unique_ptr<Binding> p(new Binding(a, a_prop, b, b_prop, std::move(converter)));
        return std::move(p);
    }
    
    ~Binding()
    {
        _a_prop_ptr->unsubscribe_on_change(this);
        _b_prop_ptr->unsubscribe_on_change(this);
    }
    
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
    ICopyable* _direct_converter_state;
    bool _converter_direction;
};

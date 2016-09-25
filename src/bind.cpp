#include "bind.h"

BindableObjectBase::BindableObjectBase() : _on_change() {}

void BindableObjectBase::fire_property_change(const char* property_name)
{
    for(auto& evnt : _on_change)
    {
        evnt.second(property_name);
    }
}

void BindableObjectBase::subscribe_on_change(void* owner, 
                         OnFieldChangeCallback on_change)
{
    _on_change[owner] = on_change;
}
void BindableObjectBase::unsubscribe_on_change(void* owner)
{
    _on_change.erase(owner);
}

ITypeDefinition* BindableObjectBase::fetch_self()
{
    if (!_self) _self = make_type_definition();
    return _self.get();
}

void Binding::a_to_b()
{
    if (_is_direct)
    {
        _direct_b->copy_from(_direct_a);
    }
    else
    {
        if (_converter)
        {
            if (_converter_state->get(0) && _converter_state->get(1))
            {
                _direct_a->copy_to(_converter_state->get(1));
                _converter->apply(*_converter_state, _converter_direction);
                _direct_b->copy_from(_converter_state->get(0));
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

void Binding::b_to_a()
{
    if (_is_direct)
    {
        _direct_a->copy_from(_direct_b);
    }
    else
    {
        if (_converter)
        {
            if (_converter_state->get(0) && _converter_state->get(1))
            {
                _direct_b->copy_to(_converter_state->get(0));
                _converter->apply(*_converter_state, !_converter_direction);
                _direct_a->copy_from(_converter_state->get(1));
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

Binding::Binding(IBindableObject* a, std::string a_prop,
                IBindableObject* b, std::string b_prop,
                std::unique_ptr<ITypeConverter> converter)
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
    _direct_a = dynamic_cast<ICopyable*>(_a_prop_ptr);
    _direct_b = dynamic_cast<ICopyable*>(_b_prop_ptr);
    
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

std::unique_ptr<Binding> Binding::bind(
        IBindableObject* a, std::string a_prop,
        IBindableObject* b, std::string b_prop,
        std::unique_ptr<ITypeConverter> converter)
{
    std::unique_ptr<Binding> p(new Binding(a, a_prop, b, b_prop, std::move(converter)));
    return std::move(p);
}

Binding::~Binding()
{
    _a_prop_ptr->unsubscribe_on_change(this);
    _b_prop_ptr->unsubscribe_on_change(this);
}

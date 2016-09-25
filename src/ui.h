#pragma once

#include <memory>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <map>

#include "../easyloggingpp/easylogging++.h"

#include "types.h"
#include "bind.h"

class IVisualElement
{
public:
    virtual Rect arrange(const Rect& origin) = 0;
    virtual void invalidate_layout() = 0;
    virtual void render(const Rect& origin) = 0;

    virtual Size2 get_size() const = 0;
    virtual Size2 get_intrinsic_size() const = 0;

    virtual void update_mouse_position(Int2 cursor) = 0;
    virtual void update_mouse_state(MouseButton button, MouseState state) = 0;
    virtual void update_mouse_scroll(Int2 scroll) = 0;

    virtual void focus(bool on) = 0;
    virtual bool is_focused() const = 0;
    virtual const std::string& get_name() const = 0;
    virtual std::string to_string() const = 0; 
    virtual const char* get_type() const = 0;
    virtual Alignment get_alignment() const = 0;
    
    virtual void set_enabled(bool on) = 0;
    virtual bool is_enabled() const = 0;
    
    virtual void set_visible(bool on) = 0;
    virtual bool is_visible() const = 0;
    
    virtual IVisualElement* find_element(const std::string& name) = 0;
    
    virtual void set_on_click(std::function<void()> on_click,
                              MouseButton button = MouseButton::left) = 0;
    virtual void set_on_double_click(std::function<void()> on_click) = 0;
    
    virtual void set_data_context(IBindableObject* dc) = 0;
    virtual IBindableObject* get_data_context() const = 0;

    virtual ~IVisualElement() {}
};

typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimePoint;

class MouseClickHandler : public virtual IVisualElement
{
public:
    MouseClickHandler();

    void update_mouse_state(MouseButton button, MouseState state) override;
    
    void update_mouse_scroll(Int2 scroll) override { // TODO
    };

    void set_on_click(std::function<void()> on_click,
                      MouseButton button = MouseButton::left) override
    {
        _on_click[button] = on_click;
    }

    void set_on_double_click(std::function<void()> on_click) override {
        _on_double_click = on_click;
    }

    bool is_pressed(MouseButton button) const
    {
        auto it = _state.find(button);
        if (it != _state.end())
            return it->second == MouseState::down;
        else
            return false;
    }

private:
    std::map<MouseButton, MouseState> _state;
    std::map<MouseButton, TimePoint> _last_update;
    std::map<MouseButton, TimePoint> _last_click;

    std::map<MouseButton, std::function<void()>> _on_click;
    std::function<void()> _on_double_click;

    const int CLICK_TIME_MS = 200;
};

class ControlBase : public virtual IVisualElement,
                    public MouseClickHandler,
                    public BindableObjectBase
{
public:
    ControlBase(std::string name,
                const Size2& position,
                const Size2& size,
                Alignment alignment)
        : _position(position), _size(size), 
          _name(name), _align(alignment), _parent(nullptr)
    {}

    Rect arrange(const Rect& origin) override;
    void invalidate_layout() override 
    {
        if (get_parent()) get_parent()->invalidate_layout();
        else LOG(INFO) << "UI Layout has invalidated! " << get_name();
    }
    
    void update_mouse_position(Int2 cursor) override {}

    Size2 get_size() const override;
    Size2 get_intrinsic_size() const override { return _size; };

    void focus(bool on) override { _focused = on; }
    bool is_focused() const override { return _focused; }
    
    const std::string& get_name() const override { return _name; }
    Alignment get_alignment() const override { return _align; }
    
    void set_enabled(bool on) override { 
        _enabled = on; 
        fire_property_change("enabled");
    }
    bool is_enabled() const override { return _enabled; }
    void set_visible(bool on) override 
    { 
        _visible = on;
        invalidate_layout();
        fire_property_change("visible");
    }
    
    bool is_visible() const override { return _visible; }
    
    IVisualElement* find_element(const std::string& name) override
    {
        if (get_name() == name) return this;
        else return nullptr;
    }
    
    IVisualElement* get_parent() { return _parent; }
    const IVisualElement* get_parent() const { return _parent; }
    void update_parent(IVisualElement* new_parent) { _parent = new_parent; }
    
    std::string to_string() const override { return get_name() + "(" + get_type() + ")"; }

    std::shared_ptr<ITypeDefinition> make_type_definition() override
    {
        DefineClass(ControlBase)->AddField(get_size)
                                 ->AddField(get_intrinsic_size)
                                 ->AddField(is_focused)
                                 ->AddField(to_string)
                                 ->AddField(get_name)
                                 ->AddField(get_alignment)
                                 ->AddProperty(is_visible, set_visible)
                                 ->AddProperty(is_enabled, set_enabled);
    }
    
    void add_binding(std::unique_ptr<Binding> binding)
    {
        _bindings.push_back(std::move(binding));
    }
    
    void set_data_context(IBindableObject* dc) override { _dc = dc; }
    IBindableObject* get_data_context() const override { return _dc; }

private:
    Size2 _position;
    Size2 _size;
    bool _focused = false;
    std::string _name;
    Alignment _align;
    bool _enabled = true;
    bool _visible = true;
    IVisualElement* _parent;
    std::vector<std::unique_ptr<Binding>> _bindings;
    IBindableObject* _dc;
};


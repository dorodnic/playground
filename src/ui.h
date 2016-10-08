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
#include "render.h"

class IVisualElement : public INotifyPropertyChanged
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

    virtual void set_focused(bool on) = 0;
    virtual bool is_focused() const = 0;
    virtual const std::string& get_name() const = 0;
    virtual std::string to_string() const = 0; 
    virtual const char* get_type() const = 0;
    virtual Alignment get_align() const = 0;
    
    virtual void set_enabled(bool on) = 0;
    virtual bool is_enabled() const = 0;
    
    virtual void set_visible(bool on) = 0;
    virtual bool is_visible() const = 0;
    
    virtual IVisualElement* find_element(const std::string& name) = 0;
    
    virtual void set_on_click(std::function<void()> on_click,
                              MouseButton button = MouseButton::left) = 0;
    virtual void set_on_double_click(std::function<void()> on_click) = 0;
    
    virtual void set_data_context(std::shared_ptr<INotifyPropertyChanged> dc) = 0;
    virtual std::shared_ptr<INotifyPropertyChanged> get_data_context() const = 0;
    
    virtual void set_render_context(const RenderContext& context) = 0;

    virtual ~IVisualElement() {}
};

typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimePoint;

class ControlBase : public IVisualElement
{
public:
    ControlBase(std::string name,
                const Size2& position,
                const Size2& size,
                Alignment alignment);
    
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
    
    Size2 get_position() const { return _position; }
    void set_position(const Size2& val) 
    { 
        _position = val; 
        fire_property_change("position");
    }

    Rect arrange(const Rect& origin) override;
    void invalidate_layout() override 
    {
        if (get_parent()) get_parent()->invalidate_layout();
        else LOG(INFO) << "UI Layout has invalidated! " << get_name();
    }
    
    void update_mouse_position(Int2 cursor) override {}

    Size2 get_size() const override;
    void set_size(const Size2& val) 
    { 
        _size = val; 
        fire_property_change("size");
    }
    
    Size2 get_intrinsic_size() const override { return _size; };

    void set_focused(bool on) override 
    { 
        _focused = on; 
        fire_property_change("focused");
    }
    bool is_focused() const override { return _focused; }
    
    const std::string& get_name() const override { return _name; }
    void set_name(const std::string& val) 
    { 
        _name = val;
        fire_property_change("name"); 
    }
    
    Alignment get_align() const override { return _align; }
    void set_align(Alignment align) 
    { 
        _align = align; 
        fire_property_change("alignment");
    }
    
    void set_enabled(bool on) override 
    { 
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
    void update_parent(IVisualElement* new_parent) 
    {
        _parent = new_parent; 
        fire_property_change("parent");
    }
    
    std::string to_string() const override { return get_name() + "(" + get_type() + ")"; }
    
    void add_binding(std::unique_ptr<Binding> binding)
    {
        _bindings.push_back(std::move(binding));
    }
    
    void set_data_context(std::shared_ptr<INotifyPropertyChanged> dc) override 
    {
        _dc = dc; 
        fire_property_change("data_context");
    }
    std::shared_ptr<INotifyPropertyChanged> get_data_context() const override 
    { 
        return _dc; 
    }
    
    void fire_property_change(const char* property_name) override
    {
        _base.fire_property_change(property_name);
    }
    void subscribe_on_change(void* owner, 
                             OnFieldChangeCallback on_change) override
    {
        _base.subscribe_on_change(owner, on_change);
    }
    void unsubscribe_on_change(void* owner) override
    {
        _base.unsubscribe_on_change(owner);
    }
    
    void set_render_context(const RenderContext& context) override
    {
        _render_context = context;
    }

protected:
    ControlBase();

    const RenderContext& get_render_context() const
    {
        return _render_context;
    }

private:
    Size2 _position = {0,0};
    Size2 _size = {0,0};
    bool _focused = false;
    std::string _name = "";
    Alignment _align = Alignment::left;
    bool _enabled = true;
    bool _visible = true;
    IVisualElement* _parent = nullptr;
    std::vector<std::unique_ptr<Binding>> _bindings;
    std::shared_ptr<INotifyPropertyChanged> _dc = nullptr;
    BindableObjectBase _base;
    
    std::map<MouseButton, MouseState> _state;
    std::map<MouseButton, TimePoint> _last_update;
    std::map<MouseButton, TimePoint> _last_click;

    std::map<MouseButton, std::function<void()>> _on_click;
    std::function<void()> _on_double_click;

    const int CLICK_TIME_MS = 200;
    RenderContext _render_context = { nullptr, nullptr };
};


template<>
struct TypeDefinition<ControlBase>
{
    static std::shared_ptr<ITypeDefinition> make() 
    {
        DefineClass(ControlBase)
                         ->AddProperty(get_position, set_position)
                         ->AddProperty(get_size, set_size)
                         ->AddField(get_intrinsic_size)
                         ->AddProperty(is_focused, set_focused)
                         ->AddField(to_string)
                         ->AddProperty(get_name, set_name)
                         ->AddProperty(get_align, set_align)
                         ->AddProperty(is_visible, set_visible)
                         ->AddProperty(is_enabled, set_enabled)
                         ->AddProperty(get_data_context, set_data_context);
    }
};


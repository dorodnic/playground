#pragma once
#include "ui.h"

class ElementAdaptor : public IVisualElement
{
public:
    ElementAdaptor(std::shared_ptr<INotifyPropertyChanged> obj)
        : _obj(obj),
          _element(dynamic_cast<IVisualElement*>(_obj.get()))
    {}
    
    void subscribe_on_change(void* owner, 
                             OnFieldChangeCallback on_change) override
    {
        _element->subscribe_on_change(owner, on_change);
    }
    void unsubscribe_on_change(void* owner) override 
    {
        _element->unsubscribe_on_change(owner);
    }
    void fire_property_change(const char* prop) override
    {
        _element->fire_property_change(prop);
    }
    
    Rect arrange(const Rect& origin) override { return _element->arrange(origin); }
    void invalidate_layout() override { _element->invalidate_layout(); }
    void render(const Rect& origin) override { _element->render(origin); }
    Size2 get_size() const override { return _element->get_size(); }
    Size2 get_intrinsic_size() const override { return _element->get_intrinsic_size(); }

    void update_mouse_position(Int2 cursor) override 
    { 
        _element->update_mouse_position(cursor);
    }
    void update_mouse_state(MouseButton button, MouseState state) override
    { 
        _element->update_mouse_state(button, state);
    }
    void update_mouse_scroll(Int2 scroll) override
    { 
        _element->update_mouse_scroll(scroll);
    }

    void set_focused(bool on) override { _element->set_focused(on); }
    bool is_focused() const override { return _element->is_focused(); }
    const std::string& get_name() const override { return _element->get_name(); }
    Alignment get_align() const override { return _element->get_align(); }
    
    void set_enabled(bool on) override { _element->set_enabled(on); }
    bool is_enabled() const override { return _element->is_enabled(); }
    
    void set_visible(bool on) override { _element->set_visible(on); }
    bool is_visible() const override { return _element->is_visible(); }
    
    IVisualElement* find_element(const std::string& name) override
    {
        return _element->find_element(name);
    }
    
    void set_on_click(std::function<void()> on_click,
                      MouseButton button = MouseButton::left) override
    {
        _element->set_on_click(on_click, button);
    }
    void set_on_double_click(std::function<void()> on_click) override
    {
        _element->set_on_double_click(on_click);
    }
    
    const char* get_type() const override { return _element->get_type(); }
    std::string to_string() const override { return _element->to_string(); }
    
    void set_data_context(std::shared_ptr<INotifyPropertyChanged> dc) override 
    { 
        _element->set_data_context(dc); 
    }
    std::shared_ptr<INotifyPropertyChanged> get_data_context() const override 
    { 
        return _element->get_data_context(); 
    }
    
    void set_render_context(const RenderContext& context) override
    {
        _element->set_render_context(context);
    }
    
    void set_font(std::shared_ptr<INotifyPropertyChanged> font) override
    {
        _element->set_font(font);
    }
    const std::shared_ptr<INotifyPropertyChanged>& get_font() const 
    {
        return _element->get_font();
    }
    
protected:
    std::shared_ptr<INotifyPropertyChanged> _obj;
    IVisualElement* _element;
};

class VisualAdaptor : public IVisualElement
{
public:
    VisualAdaptor(std::shared_ptr<INotifyPropertyChanged> obj, 
                  std::string name)
        : _obj(obj), _name(name)
    {
        if (!obj.get())
        {
            _type = "INotifyPropertyChanged";
        }
        else
        {
            _type = typeid(*obj).name();
        }
    }
    
    std::shared_ptr<INotifyPropertyChanged> get() const
    {
        return _obj;
    }
    
    void subscribe_on_change(void* owner, 
                             OnFieldChangeCallback on_change) override
    {
        _obj->subscribe_on_change(owner, on_change);
    }
    void unsubscribe_on_change(void* owner) override 
    {
        _obj->unsubscribe_on_change(owner);
    }
    void fire_property_change(const char* prop) override
    {
        _obj->fire_property_change(prop);
    }
    
    Rect arrange(const Rect& origin) override { return Rect(); }
    void invalidate_layout() override { }
    void render(const Rect& origin) override { _obj->update(); }
    Size2 get_size() const override { return Size2(); }
    Size2 get_intrinsic_size() const override { return Size2(); }

    void update_mouse_position(Int2 cursor) override {}
    void update_mouse_state(MouseButton button, MouseState state) override {}
    void update_mouse_scroll(Int2 scroll) override {}

    void set_focused(bool on) override { }
    bool is_focused() const override { return false; }
    const std::string& get_name() const override { return _name; }
    Alignment get_align() const override { return Alignment::left; }
    
    void set_enabled(bool on) override { }
    bool is_enabled() const override { return false; }
    
    void set_visible(bool on) override { }
    bool is_visible() const override { return false; }
    
    IVisualElement* find_element(const std::string& name) override
    {
        if (name == _name) 
        {
            return this;
        }
        else return nullptr;
    }
    
    void set_on_click(std::function<void()> on_click,
                      MouseButton button = MouseButton::left) override {}
    void set_on_double_click(std::function<void()> on_click) override {}
    
    const char* get_type() const override { return _type.c_str(); }
    std::string to_string() const override { return _type; }
    
    void set_data_context(std::shared_ptr<INotifyPropertyChanged> dc) override {}
    std::shared_ptr<INotifyPropertyChanged> get_data_context() const override 
    {
        return nullptr;
    }
    
    void set_render_context(const RenderContext& context) override {}
    
    void set_font(std::shared_ptr<INotifyPropertyChanged> font) override {}
    const std::shared_ptr<INotifyPropertyChanged>& get_font() const 
    {
        return _font;
    }
    
private:
    std::shared_ptr<INotifyPropertyChanged> _obj;
    std::shared_ptr<INotifyPropertyChanged> _font = nullptr;
    std::string _name;
    std::string _type;
};

template<>
struct TypeDefinition<VisualAdaptor>
{
    static std::shared_ptr<ITypeDefinition> make() 
    {
        DefineClass(VisualAdaptor)->AddField(get_type)->AddField(get_name);
    }
};

class MarginAdaptor : public ElementAdaptor
{
public:
    MarginAdaptor(std::shared_ptr<INotifyPropertyChanged> element,
                  Margin margin)
        : ElementAdaptor(element), _margin(margin)
    {}
    
    Rect arrange(const Rect& origin) override 
    { 
        return _margin.apply(_element->arrange((-_margin).apply(origin))); 
    }
    
    void render(const Rect& origin) override 
    { 
        _element->render((-_margin).apply(origin)); 
    }

    Size2 get_size() const override 
    {
        return _margin.apply(_element->get_size()); 
    }
    Size2 get_intrinsic_size() const override 
    { 
        return _margin.apply(_element->get_intrinsic_size());
    }
    
private:
    Margin _margin;
};

class VisibilityAdaptor : public ElementAdaptor
{
public:
    VisibilityAdaptor(std::shared_ptr<INotifyPropertyChanged> element)
        : ElementAdaptor(element)
    {}
    
    Rect arrange(const Rect& origin) override 
    { 
        Rect def = { {0,0}, {0,0} };
        return _element->is_visible() ? _element->arrange(origin) : def;
    }
    
    void render(const Rect& origin) override 
    { 
        if (_element->is_visible()) _element->render(origin);
    }

    Size2 get_size() const override 
    {
        Size2 def = { 0.0f, 0.0f };
        return _element->is_visible() ? _element->get_size() : def;
    }
    Size2 get_intrinsic_size() const override 
    { 
        Size2 def = { 0.0f, 0.0f };
        return _element->is_visible() ? _element->get_intrinsic_size() : def;
    }
    
    void update_mouse_position(Int2 cursor) override 
    { 
        if (_element->is_visible()) _element->update_mouse_position(cursor);
    }
    void update_mouse_state(MouseButton button, MouseState state) override
    { 
        if (_element->is_visible()) _element->update_mouse_state(button, state);
    }
    void update_mouse_scroll(Int2 scroll) override
    { 
        if (_element->is_visible()) _element->update_mouse_scroll(scroll);
    }
};

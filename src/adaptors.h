#pragma once
#include "ui.h"

class ElementAdaptor : public IVisualElement
{
public:
    ElementAdaptor(std::shared_ptr<IVisualElement> element)
        : _element(element)
    {}
    
    Rect arrange(const Rect& origin) override { return _element->arrange(origin); }
    void invalidate_layout() override { _element->invalidate_layout(); }
    void render(const Rect& origin) override { _element->render(origin); }
    const Margin& get_margin() const override { return _element->get_margin(); }
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

    void focus(bool on) override { _element->focus(on); }
    bool is_focused() const override { return _element->is_focused(); }
    const std::string& get_name() const override { return _element->get_name(); }
    Alignment get_alignment() const override { return _element->get_alignment(); }
    
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
    
protected:
    std::shared_ptr<IVisualElement> _element;
};

class MarginAdaptor : public ElementAdaptor
{
public:
    MarginAdaptor(std::shared_ptr<IVisualElement> element,
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

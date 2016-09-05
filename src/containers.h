#pragma once
#include "ui.h"

enum class Orientation
{
    vertical,
    horizontal
};

typedef std::unordered_map<IVisualElement*, std::pair<int, Size>> SizeMap;
typedef std::vector<std::shared_ptr<IVisualElement>> Elements;

class StackPanel;

class ISizeCalculator
{
public:
    virtual SizeMap calc_sizes(const StackPanel* sender,
                               const Rect& arrangement) const = 0;
};

class Container : public ControlBase
{
public:
    Container(std::string name,
              const Size2& position,
              const Size2& size,
              const Margin& margin,
              Alignment alignment)
        : ControlBase(name, position, size, margin, alignment), 
          _on_items_change([](){}),
          _on_focus_change([](){})
    {}
    
    void update_mouse_position(Int2 cursor) override
    {
        if (_focused)
        {
            _focused->update_mouse_position(cursor);
        }
    }
    
    void update_mouse_state(MouseButton button, MouseState state) override
    {
        if (_focused)
        {
            _focused->update_mouse_state(button, state);
        }
    }

    void invalidate_layout() override
    {
        _origin = { { 0, 0 }, { 0, 0 } };
        if (get_parent()) ControlBase::invalidate_layout();
    }

    virtual void add_item(std::shared_ptr<IVisualElement> item);

    void set_items_change(std::function<void()> on_change)
    {
        _on_items_change = on_change;
    }
    
    void set_focus_change(std::function<void()> on_change)
    {
        _on_focus_change = on_change;
    }
    
    const Elements& get_elements() const { return _content; }
    const Rect& get_arrangement() const { return _arrangement; }
    
    bool update_layout(const Rect& origin)
    {
        if (_origin == origin) return false;
        
        _arrangement = arrange(origin);
        _origin = origin;
        return true;
    }
    
    void set_focused_child(IVisualElement* focused) { 
        _focused = focused; 
        _on_focus_change();
    }
    
    const IVisualElement* get_focused_child() const { return _focused; }
    IVisualElement* get_focused_child() { return _focused; }
    
    IVisualElement* find_element(const std::string& name) override
    {
        if (get_name() == name) return this;
        
        for (auto& e : get_elements())
        {
            auto r = e->find_element(name);
            if (r) return r;
        }
        
        return nullptr;
    }

private:
    IVisualElement* _focused = nullptr;

    Elements _content;
    Rect _origin;
    Rect _arrangement;

    std::function<void()> _on_items_change;
    std::function<void()> _on_focus_change;
};

class StackPanel : public Container
{
public:
    StackPanel(std::string name,
              const Size2& position,
              const Size2& size,
              const Margin& margin,
              Alignment alignment,
              Orientation orientation,
              ISizeCalculator* resizer = nullptr)
        : Container(name, position, size, margin, alignment), 
          _orientation(orientation),
          _resizer(resizer)
    {}
    
    const char* get_type() const override { return "StackPanel"; }
    
    void update_mouse_position(Int2 cursor) override;
    
    static SizeMap calc_sizes(Orientation orientation,
                              const Elements& content,
                              const Rect& arrangement);

    SizeMap calc_local_sizes(const Rect& arrangement) const
    {
        return calc_sizes(_orientation,
                          get_elements(),
                          arrangement);
    }

    SizeMap calc_global_sizes(const Rect& arrangement) const
    {
        if (_resizer) {
            return _resizer->calc_sizes(this, arrangement);
        } else {
            return calc_local_sizes(arrangement);
        }
    }

    Size2 get_intrinsic_size() const override;

    void render(const Rect& origin) override;
    
    Orientation get_orientation() const { return _orientation; }

private:
    SizeMap _sizes;
    ISizeCalculator* _resizer;
    Orientation _orientation;
};

typedef std::unordered_map<IVisualElement*,Int2> SimpleSizeMap;

class Panel : public Container
{
public:
    Panel(std::string name,
          const Size2& position,
          const Size2& size,
          const Margin& margin,
          Alignment alignment)
        : Container(name, position, size, margin, alignment)
    {}
    
    const char* get_type() const override { return "Panel"; }
                   
    Size2 get_intrinsic_size() const override;

    void render(const Rect& origin) override;
    
    static SimpleSizeMap calc_size_map(const Elements& content,
                                       const Rect& arrangement);
};

class PageView : public Container
{
public:
    PageView(std::string name,
             const Size2& position,
             const Size2& size,
             const Margin& margin,
             Alignment alignment)
        : Container(name, position, size, margin, alignment)
    {
        set_focus_change([this]() { invalidate_layout(); });
    }
    
    const char* get_type() const override { return "PageView"; }

    Size2 get_intrinsic_size() const override;

    void render(const Rect& origin) override;
};

class Grid : public StackPanel, public ISizeCalculator
{
public:
    Grid( std::string name,
          const Size2& position,
          const Size2& size,
          const Margin& margin,
          Alignment alignment,
          Orientation orientation)
        : StackPanel(name, position, size, margin, 
                     alignment, orientation),
          _current_line(nullptr)
    {
        commit_line();
    }
    
    const char* get_type() const override { return "Grid"; }

    virtual void add_item(std::shared_ptr<IVisualElement> item)
    {
        auto control = dynamic_cast<ControlBase*>(item.get());
        if (control) control->update_parent(_current_line.get());
        _current_line->add_item(item);
    }

    void commit_line();


    SizeMap calc_sizes(const StackPanel* sender,
                       const Rect& arrangement) const override;

private:
    std::shared_ptr<StackPanel> _current_line;
    std::vector<std::shared_ptr<StackPanel>> _lines;
};

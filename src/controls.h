#pragma once
#include "ui.h"

class TextBlock : public ControlBase
{
public:
    TextBlock(std::string name,
            std::string text,
            Alignment alignment,
            const Size2& position,
            const Size2& size,
            const Color3& color)
        : ControlBase(name, position, size, alignment), 
          _color(color), _text(text)
    {}
    
    TextBlock() {}

    Size2 get_intrinsic_size() const override;

    void render(const Rect& origin) override;
    
    void set_text(std::string text);
    
    const std::string& get_text() const { return _text; }
    
    const char* get_type() const override { return "TextBlock"; }
    
    void set_color(Color3 color) { 
        _color = color; 
        fire_property_change("color");
    }
    const Color3& get_color() const { return _color; }
    

private:
    Color3 _color;
    std::string _text;
};

class Button : public ControlBase
{
public:
    Button(std::string name,
           std::string text,
           Alignment text_alignment,
           const Color3& text_color,
           Alignment alignment,
           const Size2& position,
           const Size2& size,
           const Color3& color)
        : ControlBase(name, position, size, alignment),
          _text_block(name, text, text_alignment, 
                      {0, 0}, { 1.0f, 1.0f }, text_color), 
          _color(color)
    {
        _text_block.update_parent(this);
    }
    
    Button() {}
    
    const char* get_type() const override { return "Button"; }
    
    Size2 get_intrinsic_size() const override;

    void render(const Rect& origin) override;
    
    void set_color(Color3 color) { 
        _color = color; 
        fire_property_change("color");
    }
    const Color3& get_color() const { return _color; }
    
    void set_text_color(Color3 color) { 
        _text_block.set_color(color);
        fire_property_change("text_color");
    }
    const Color3& get_text_color() const { return _text_block.get_color(); }
    
    void set_text(std::string text) 
    { 
        _text_block.set_text(text);
        fire_property_change("text");
    }
    
    const std::string& get_text() const { return _text_block.get_text(); }

private:
    Color3 _color;
    TextBlock _text_block;
};

class Slider : public ControlBase
{
public:
    Slider(std::string name,
            Alignment alignment,
            const Size2& position,
            const Size2& size,
            const Color3& color,
            const Color3& text_color,
            Orientation orientation,
            float min, float max, float step, float value)
        : ControlBase(name, position, size, alignment), 
          _color(color), _text_color(text_color), _orientation(orientation),
          _min(min), _max(max), _value(value), _step(step)         
    {}
    
    Slider() {}

    const char* get_type() const override { return "Slider"; }
    
    Size2 get_intrinsic_size() const override;

    void render(const Rect& origin) override;
    
    void update_mouse_position(Int2 cursor) override;
    void update_mouse_state(MouseButton button, MouseState state) override;
    
    void set_focused(bool on) override 
    {
        if (!on) {
            
            _dragging = false; 
            _dragger_ready = false;
        }
        ControlBase::set_focused(on);
    }
    
    void set_value(float val) { 
        _value = val; 
        fire_property_change("value");
    }
    float get_value() const { return _value; }
    
    void set_min(float val) { 
        _min = val; 
        fire_property_change("min");
    }
    float get_min() const { return _min; }
    
    void set_max(float val) { 
        _max = val; 
        fire_property_change("max");
    }
    float get_max() const { return _max; }
    
    void set_step(float val) { 
        _step = val; 
        fire_property_change("step");
    }
    float get_step() const { return _step; }
    
    void set_show_ticks(bool val) { 
        _show_ticks = val; 
        fire_property_change("show_ticks");
    }
    bool get_show_ticks() const { return _show_ticks; }
    
    void set_orientation(Orientation val) { 
        _orientation = val; 
        fire_property_change("orientation");
    }
    Orientation get_orientation() const { return _orientation; }
    
    void set_color(const Color3& val) { 
        _color = val; 
        fire_property_change("color");
    }
    const Color3& get_color() const { return _color; }
    
    void set_text_color(const Color3& val) { 
        _text_color = val; 
        fire_property_change("text_color");
    }
    const Color3& get_text_color() const { return _text_color; }

private:
    float _min;
    float _max;
    float _step;
    float _value;
    bool _show_ticks;
    Orientation _orientation;
    Color3 _text_color;
    Color3 _color;
    
    bool _dragger_ready = false;
    bool _dragging = false;
    Rect _rect;
};

template<>
struct TypeDefinition<Slider>
{
    static std::shared_ptr<ITypeDefinition> make() 
    {
        ExtendClass(Slider, ControlBase)
             ->AddProperty(get_value, set_value)
             ->AddProperty(get_min, set_min)
             ->AddProperty(get_max, set_max)
             ->AddProperty(get_step, set_step)
             ->AddProperty(get_show_ticks, set_show_ticks)
             ->AddProperty(get_color, set_color)
             ->AddProperty(get_text_color, set_text_color)
             ;
    }
};


template<>
struct TypeDefinition<Button>
{
    static std::shared_ptr<ITypeDefinition> make() 
    {
        ExtendClass(Button, ControlBase)
             ->AddProperty(get_text, set_text)
             ->AddProperty(get_color, set_color)
             ->AddProperty(get_text_color, set_text_color);
    }
};

template<>
struct TypeDefinition<TextBlock>
{
    static std::shared_ptr<ITypeDefinition> make() 
    {
        ExtendClass(TextBlock, ControlBase)
             ->AddProperty(get_text, set_text)
             ->AddProperty(get_color, set_color)
             ;
    }
};
             


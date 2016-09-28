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

    Size2 get_intrinsic_size() const override;

    void render(const Rect& origin) override;
    
    void set_text(std::string text);
    
    const std::string& get_text() const { return _text; }
    
    const char* get_type() const override { return "TextBlock"; }

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
    
    const char* get_type() const override { return "Button"; }
    
    Size2 get_intrinsic_size() const override;

    void render(const Rect& origin) override;
    
    void set_color(Color3 color) { 
        _color = color; 
        fire_property_change("color");
    }
    const Color3& get_color() const { return _color; }
    
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

    const char* get_type() const override { return "Slider"; }
    
    Size2 get_intrinsic_size() const override;

    void render(const Rect& origin) override;
    
    void update_mouse_position(Int2 cursor) override;
    void update_mouse_state(MouseButton button, MouseState state) override;
    
    void focus(bool on) override 
    {
        if (!on) {
            
            _dragging = false; 
            _dragger_ready = false;
        }
        ControlBase::focus(on);
    }
    
    void set_value(float val) { 
        _value = val; 
        fire_property_change("value");
    }
    float get_value() const { return _value; }

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
             ->AddProperty(get_value, set_value);
    }
};


template<>
struct TypeDefinition<Button>
{
    static std::shared_ptr<ITypeDefinition> make() 
    {
        ExtendClass(Button, ControlBase)
             ->AddProperty(get_text, set_text)
             ->AddProperty(get_color, set_color);
    }
};

template<>
struct TypeDefinition<TextBlock>
{
    static std::shared_ptr<ITypeDefinition> make() 
    {
        ExtendClass(TextBlock, ControlBase)
             ->AddProperty(get_text, set_text);
    }
};
             


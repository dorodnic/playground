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
            const Margin& margin,
            const Color3& color)
        : ControlBase(name, position, size, margin, alignment), 
          _color(color), _text(text)
    {}

    Size2 get_intrinsic_size() const override;

    void render(const Rect& origin) override;
    
    void set_text(std::string text) 
    { 
        _text = text; 
        ControlBase::invalidate_layout();
    }
    
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
           const Margin& margin,
           const Color3& color)
        : ControlBase(name, position, size, margin, alignment),
          _text_block(name, text, text_alignment, 
                      {0, 0}, { 1.0f, 1.0f }, 0, text_color), 
          _color(color)
    {
        _text_block.update_parent(this);
    }
    
    const char* get_type() const override { return "Button"; }
    
    Size2 get_intrinsic_size() const override;

    void render(const Rect& origin) override;
    
    void set_color(Color3 color) { _color = color; }
    const Color3& get_color() const { return _color; }
    
    void set_text(std::string text) 
    { 
        _text_block.set_text(text);
    }
    
    const std::string& get_text() const { return _text_block.get_text(); }

private:
    Color3 _color;
    TextBlock _text_block;
};

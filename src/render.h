#pragma once

class FontRenderer;
class Flat2dRenderer;

struct RenderContext
{
    FontRenderer* font_renderer;
    Flat2dRenderer* flat2d_renderer;
};
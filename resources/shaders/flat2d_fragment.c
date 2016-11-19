#version 330 core

out vec4 color;

varying vec2 pixel_pos;

uniform vec3 rect_color;
uniform vec2 rect_size;

uniform float rounding_size;

float calc_a(vec2 t)
{
    t.x = 1 - t.x;
    t.y = 1 - t.y;
    return 1 - smoothstep(0.9, 1, sqrt(t.x*t.x + t.y*t.y));
}

void main()
{
    /*if(((pixel_pos.x < 1 || rect_size.x - pixel_pos.x < 2) && mod((pixel_pos.y) / 4, 2) < 1) ||
       ((pixel_pos.y < 1 || rect_size.y - pixel_pos.y < 2) && mod((pixel_pos.x) / 4, 2) < 1))
    {
        color.xyz = vec3(1);
        color.a = 1;
    }
    else*/
    {
        color.xyz = rect_color;
        
        if (rounding_size > 0)
        {
            vec2 t = vec2(1, 1);
        
            if (pixel_pos.x < rounding_size && pixel_pos.y < rounding_size)
            {
                t = pixel_pos / rounding_size;
            }
            else if (rect_size.x - pixel_pos.x < rounding_size && pixel_pos.y < rounding_size)
            {
                t = vec2(rect_size.x - pixel_pos.x, pixel_pos.y) / rounding_size;
            }
            else if (pixel_pos.x < rounding_size && (rect_size.y - pixel_pos.y < rounding_size))
            {
                t = vec2(pixel_pos.x, rect_size.y - pixel_pos.y) / rounding_size;
            }
            else if (rect_size.x - pixel_pos.x < rounding_size && rect_size.y - pixel_pos.y < rounding_size)
            {
                t = (rect_size - pixel_pos) / rounding_size;
            }
            
            color.a = calc_a(t);
        }
        else color.a = 1;
    }
}


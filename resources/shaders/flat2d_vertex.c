#version 330 core

layout(location = 0) in vec2 vertex_pos;
layout(location = 1) in vec2 vertex_rel_pos;

varying vec2 pixel_pos;

uniform vec2 screen_size;

void main()
{
    pixel_pos = vertex_rel_pos;
    gl_Position.xy = ((vec2(1,-1) * vertex_pos.xy) / screen_size.xy) * 2 + vec2(-1,1);
    gl_Position.w = 1.0;
	gl_Position.z = 0.0;
}


#version 330 core

layout(location = 0) in vec2 vertex_pos;
layout(location = 1) in vec2 vertex_uv;

out vec2 uv;

uniform vec2 screen_size;
uniform vec2 text_position;

uniform float size_ratio;

void main()
{
    gl_Position.xy = ((vec2(1,-1) * text_position + size_ratio * vertex_pos.xy) / screen_size.xy) * 2 + vec2(-1,1);
    gl_Position.w = 1.0;
	gl_Position.z = 0.0;
	uv = vertex_uv;
}


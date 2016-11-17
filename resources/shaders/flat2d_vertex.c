#version 330 core

layout(location = 0) in vec2 vertex_pos;

uniform vec2 screen_size;

void main()
{
    gl_Position.xy = ((vec2(1,-1) * vertex_pos.xy) / screen_size.xy) * 2 + vec2(-1,1);
    gl_Position.w = 1.0;
	gl_Position.z = 0.0;
}


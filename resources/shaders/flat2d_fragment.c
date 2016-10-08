#version 330 core

out vec4 color;

uniform vec3 rect_color;

void main()
{
	color.xyz = rect_color;
	color.a = 1;
}


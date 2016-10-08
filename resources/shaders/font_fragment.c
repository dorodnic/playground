#version 330 core

out vec4 color;

in vec2 uv;

uniform sampler2D smapler;

uniform float sdf_width;
uniform float sdf_edge;

uniform vec3 font_color;

void main()
{
	float dist = 1 - texture(smapler, uv).a;
	float alpha = 1 - smoothstep(sdf_width, sdf_width + sdf_edge, dist);

	color.xyz = font_color;
	color.a = alpha;
}


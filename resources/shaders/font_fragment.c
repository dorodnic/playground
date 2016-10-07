#version 330 core

out vec4 color;

in vec2 UV;

uniform sampler2D myTextureSampler;

const float width = 0.5;
const float edge = 0.1;

uniform vec3 font_color;

void main()
{
	float dist = 1 - texture( myTextureSampler, UV ).a;
	float alpha = 1 - smoothstep(width, width + edge, dist);

	color.xyz = font_color;
	color.a = alpha;
}


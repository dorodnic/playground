#version 330 core

layout(location = 0) in vec2 vertexPosition_modelspace;
layout(location = 1) in vec2 vertexUV;

out vec2 UV;

void main()
{
    gl_Position.xy = vertexPosition_modelspace;
    gl_Position.w = 1.0;
	gl_Position.z = 0.0;
	UV = vertexUV;
}


#version 330 core

layout(location = 0) in vec2 vertexPosition_modelspace;

void main()
{
    gl_Position.xy = vertexPosition_modelspace;
    gl_Position.w = 1.0;
	gl_Position.z = 0.0;
}


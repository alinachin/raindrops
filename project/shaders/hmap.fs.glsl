#version 330 core

in float height;
out vec4 outputColor;

void main()
{
	outputColor = vec4(vec3(height), 1.0f);
}
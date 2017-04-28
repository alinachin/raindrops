#version 330 core

in vec2 position;

uniform mat4 mvp;

void main()
{
	gl_Position = mvp * vec4(position, 0.0f, 1.0f);
}
#version 330 core

layout (location = 0) in vec2 vertPos;
layout (location = 1) in vec2 cellPos;
layout (location = 2) in float cellHeight;
out float height;

uniform mat4 mvp;
uniform float cellSize;

void main()
{
	vec2 position = cellPos + vertPos * cellSize;
	gl_Position = mvp * vec4(position, 0.0f, 1.0f);
	height = min(1.0f, cellHeight * 4e-1f);
}
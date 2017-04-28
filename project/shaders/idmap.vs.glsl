#version 330 core 

layout (location = 0) in vec2 vertPos;
layout (location = 1) in vec2 cellPos;
layout (location = 2) in uint id;
out vec3 color;

uniform mat4 mvp;
uniform float cellSize;
uniform vec3[6] colors;

void main()
{
	vec2 position = cellPos + vertPos * cellSize;
	gl_Position = mvp * vec4(position, 0.0f, 1.0f);
	uint idColor = id == 0U ? 0U : id % 5U + 1U;
	color = colors[idColor];
}
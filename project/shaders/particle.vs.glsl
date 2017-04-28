#version 330 core

#define M_PI 3.1415926535897932384626433832795

layout (location = 0) in vec2 vertPos;
layout (location = 1) in vec2 center;
layout (location = 2) in float mass;

uniform mat4 mvp;
void main()
{
	float radius = pow( (3.f/(2*M_PI) * mass), 1.f/3.f );
	vec2 position = center + vertPos * radius * 2.0f;
	gl_Position = mvp * vec4(position, 0.0f, 1.0f);
}
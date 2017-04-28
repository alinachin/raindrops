#ifndef INCLUDE_PARTICLE
#define INCLUDE_PARTICLE

#pragma once

#include <glm/vec2.hpp>
#include "SimpleID.hpp"

struct Particle {
	glm::vec2 p;
	glm::vec2 v;
	float m;
	bool moving;
	bool active;
	idnum id;
	float tau;
	//glm::vec2 subp[5];
	//int subn;
};

#endif
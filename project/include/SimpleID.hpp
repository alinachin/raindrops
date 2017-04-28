#ifndef INCLUDE_SIMPLEID
#define INCLUDE_SIMPLEID

#pragma once

#include <atlas/gl/GL.hpp>

typedef GLuint idnum;

class SimpleID {
public:
	SimpleID();
	idnum getID();
private:
	idnum nextID;
};

#endif
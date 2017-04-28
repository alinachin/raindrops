#ifndef INCLUDE_GRIDCELL
#define INCLUDE_GRIDCELL

#pragma once

#include <vector>
#include <glm/vec2.hpp>
#include "SimpleID.hpp"

struct GridParams {
	float angle;  // [0, 90] degrees
	float res;  // (mm) side of each cell
	int m;  // number of cells horizontally
	int n;  // number of cells vertically
};

struct Region {
	float height;
	float affinity;
};

class GridCells {
public:
	GridCells(GridParams const& g);

	const int m, n;  // number of cells
	const float w, h;  // worldspace width and height of entire grid
	const float res;  // width of each cell
	const long dim;  // m * n

	std::vector<glm::vec2> center;  // modelspace
	std::vector<float> height;
	std::vector<idnum> id;  // 0 means no particle

	// get random affinity coefficient
	static float getAffinity();

	// update ID map after a merge
	void floodFill(long seed, idnum to, idnum from);

	// get grid index from modelspace coords
	long index(glm::vec2 pos);

	// updates height AND id maps
	// position given separately (e.g. interpolation for water flows)
	void placeHemisphere(glm::vec2 pos, float radius, idnum id);

	// returns row-major index for grid, or -1 if out of bounds
	long ROWSAFE(int i, int j);

	// (SLOW) get avg properties from 3x3 region centered on (i, j)
	Region region3x3(int i, int j);

	void erode();

	void smooth();

private:
	std::vector<float> affinity;
	std::vector<float> height2;
	std::vector<idnum> id2;
};


#endif
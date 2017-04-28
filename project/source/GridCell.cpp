#include "GridCell.hpp"
#include <set>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>

#define ROWM(x, y)  ((x) + (y)*m)

GridCells::GridCells(GridParams const& g) :
	m(g.m),
	n(g.n),
	w(g.m * g.res),
	h(g.n * g.res),
	res(g.res),
	dim((long)g.m * g.n)
{
	center = std::vector<glm::vec2>(m*n, glm::vec2{});
	height = std::vector<float>(m*n, 0.0f);
	height2 = std::vector<float>(height);
	affinity = std::vector<float>(height);
	id = std::vector<idnum>(m*n, 0U);
	id2 = std::vector<idnum>(id);

	float halfres = res / 2;
	for (int j = 0; j < n; j++)
		for (int i = 0; i < m; i++) {
			float x = 2 * halfres * i + halfres;
			float y = h - (2 * halfres * j + halfres);
			center[ROWM(i, j)] = { x, y };
		}
	for (auto& a : affinity)
		a = getAffinity();
}

float GridCells::getAffinity()
{
	return glm::clamp(glm::gaussRand<float>(0.5f, 1.5f), 0.0f, 1.0f);
}

void GridCells::floodFill(long seed, idnum to, idnum from)
{
	std::set<long> neighbours;
	neighbours.insert(seed);

	while (!neighbours.empty()) {
		auto index = *neighbours.begin();
		id[index] = to;
		int x = index % m;
		int y = index / m;
		int c;
		for (int a = -1; a <= 1; a+=2) {
			c = ROWSAFE(x + a, y);
			if ((c != -1) && (id[c] == from))
				neighbours.insert(c);
		}
		for (int b = -1; b <= 1; b+=2) {
			c = ROWSAFE(x, y + b);
			if ((c != -1) && (id[c] == from))
				neighbours.insert(c);
		}
		neighbours.erase(index);
	}
}

long GridCells::index(glm::vec2 pos)
{
	int i, j;
	i = (int)glm::floor(pos.x / res);
	j = (int)glm::floor((h - pos.y) / res);
	return ROWM(i, j);
}

void GridCells::placeHemisphere(glm::vec2 pos, float radius, idnum ID)
{
	// get grid coords
	long k = index(pos);
	int i = k % m;
	int j = k / m;

	// moved radius calculation out of this function

	// put radius in terms of cells (X by X block)
	int cr = (int)glm::ceil(radius / res);

	// go through possible cells w/ actual radius
	for (int b = j - cr; b <= j + cr; b++)
		for (int a = i - cr; a <= i + cr; a++) {
			long c = ROWSAFE(a, b);
			if (c != -1) {
				// cell center-particle center
				float d = radius * radius - std::pow(glm::distance(center[c], pos), 2);
				if (d > 0)
					if (height[c] < glm::sqrt(d)) {  // update height
						height[c] = glm::sqrt(d);
						id[c] = ID;
					}
			}
		}
}

long GridCells::ROWSAFE(int x, int y)
{
	return (x < 0) || (x >= m) || (y < 0) || (y >= n) ? -1 : x + (long)y*m;
}

Region GridCells::region3x3(int x, int y)
{
	long index = ROWSAFE(x, y);
	if (index == -1)
		return Region{ 0.0f, 0.0f };

	float hv = 0.0f;
	float aff = 0.0f;
	int N = 0;  // how many valid cells

	for (int b = y - 1; b <= y + 1; b++)
		for (int a = x - 1; a <= x + 1; a++) {
			auto k = ROWSAFE(a, b);
			if (k != -1) {
				N++;
				hv += height[k];
				aff += affinity[k];
			}
		}

	return Region{ hv / N, aff / N };
}

void GridCells::erode()
{
	for (int j = 0; j < n; j++) {
		for (int i = 0; i < m - 1; i++) {
			// each pair left-to-right
			float* hv = &(height[ROWM(i, j)]);
			if ((*hv > 0.0f) && (*(hv + 1) == 0.0f)) {
				*hv = 0.0f;
				id[ROWM(i, j)] = 0;
				i++;  // skip
			}
		}
		for (int i = m - 2; i >= 0; i--) {
			// each pair right-to-left
			float* hv = &(height[ROWM(i, j)]);
			if ((*hv == 0.0f) && (*(hv + 1) > 0.0f)) {
				*(hv + 1) = 0.0f;
				id[ROWM(i + 1, j)] = 0;
				i--;  // skip
			}
		}
	}
}

void GridCells::smooth()
{
	// 3x3 box blur
	// col blur
	for (int i = 0; i < m; i++) {
		float acc = height[ROWM(i, 0)] + height[ROWM(i, 1)];
		float tail = 0.0f;
		height2[ROWM(i, 0)] = 2 * height[ROWM(i, 0)] + height[ROWM(i, 1)];
		for (int j = 1; j < n - 1; j++) {
			float head = height[ROWM(i, j + 1)];
			acc = acc - tail + head;
			height2[ROWM(i, j)] = acc;
			tail = height[ROWM(i, j - 1)];
		}
		height2[ROWM(i, n - 1)] = height[ROWM(i, n - 2)] + 2 * height[ROWM(i, n - 1)];
	}
	std::swap(height2, height);  // constant complexity

	// row blur
	for (int j = 0; j < n; j++) {
		float left = 0.0f;
		float acc = height[ROWM(0, j)] + height[ROWM(1, j)];
		height2[ROWM(0, j)] =
			(2 * height[ROWM(0, j)] + height[ROWM(1, j)]) / 9;
		for (int i = 1; i < m - 1; i++) {
			float right = height[ROWM(i + 1, j)];
			acc = acc - left + right;  // acc - previous left + new right
			height2[ROWM(i, j)] = acc / 9;
			left = height[ROWM(i - 1, j)];
		}
		height2[ROWM(m - 1, j)] =
			(height[ROWM(m - 2, j)] + 2 * height[ROWM(m - 1, j)]) / 9;
	}
	std::swap(height2, height);

	// low-pass filter (0.01)
	for (auto& hv : height) {
		if (hv < 0.01f)
			hv = 0.0f;
	}

	// ID smoothing

	// 2D dilation
	for (int y = 0; y < n - 1; y++)
		for (int x = 0; x < m - 1; x++) {
			// fill cells up and to the left
			if (id[ROWM(x, y)] == 0)
				if (id[ROWM(x, y + 1)] != 0)
					id[ROWM(x, y)] = id[ROWM(x, y + 1)];
				else if (id[ROWM(x + 1, y)] != 0)
					id[ROWM(x, y)] = id[ROWM(x + 1, y)];
		}
	for (int y = n - 2; y >= 0; y--)
		for (int x = m - 2; x >= 0; x--) {
			// fill cells down and to the right
			if (id[ROWM(x + 1, y + 1)] == 0)
				if (id[ROWM(x + 1, y)] != 0)
					id[ROWM(x + 1, y + 1)] = id[ROWM(x + 1, y)];
				else if (id[ROWM(x, y + 1)] != 0)
					id[ROWM(x + 1, y + 1)] = id[ROWM(x, y + 1)];
		}
	
	// threshold
	for (long k = 0; k < height.size(); ++k)
		if (height[k] < 0.5f)
			id[k] = 0;
}

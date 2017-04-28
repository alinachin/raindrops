#ifndef INCLUDE_GRID
#define INCLUDE_GRID

#pragma once

#include <atlas/utils/Geometry.hpp>
#include "GridCell.hpp"
#include "Particle.hpp"
#include "SimpleID.hpp"
#include <unordered_map>

typedef std::unordered_multimap<idnum, idnum> parentmap;

struct ParticleParams {
	long maxParticles;
	float critMass;  // (mg)
	float minInitialMass;
	float maxInitialMass;
	float meander;  // meandering max angle in radians
};

struct DemoFlags {
	bool ERODE;
	bool SMOOTH;
	bool RESIDU;
	bool MERGE;
	bool showParticles;
	bool showIDMap;
};

class Sim : public atlas::utils::Geometry  {
public:
	Sim(GridParams const& p, ParticleParams const& g);
	~Sim();
	
	void updateGeometry(atlas::utils::Time const& t) override;
	void renderGeometry(atlas::math::Matrix4 projection,
		atlas::math::Matrix4 view) override;

	void clearParticles();

	static glm::vec2 dirSE, dirSW, dirS;
	enum class Direction { SE, SW, S };
	DemoFlags flags;
		
private:
	GLuint gVao, hmapVao, pVao, idmapVao;
	GLuint gVbo;  // VBO for grid positions
	GLuint bbVbo;  // VBO for billboard vertices (2 triangles)
	GLuint hmapVbo;  // VBO for grid heights
	GLuint pVbo;  // VBO for particle data
	GLuint idmapVbo;  // VBO for ID map (merging areas)

	float angle;  // angle of windscreen
	glm::vec2 g;  // gravitational acc, scaled according to angle
	float critMass;  // minimum mass for a particle to start moving
	float minInitialMass, maxInitialMass;
	float maxTau;  // max time until a drop leaves a residual particle
	float meander;  // max angle of deviation

	GridCells grid;
	std::vector<Particle> particles;
	std::unordered_map<idnum, Particle*> plookup;
	parentmap donotmerge;
	
	long maxParticles;
	SimpleID idGen;
	glm::vec3 colors[6];  // predefined colors for id map

	// initializing functions
	void gridSetupGl();
	void hmapSetupGl();
	void pSetupGl();
	void idmapSetupGl();

	// calculate direction & speed of moving particle
	void moveParticle(Particle& p, float dt);
	
	// given time accumulator & timestep, whether to leave residual particle
	bool residual(float tau, float dt);

	// set active flag & assign ID
	void activateParticle(Particle& p);

	// remove non-active particles from memory
	void cleanupParticles();

	void generate();

	void updateMovingParticles(float dt);

	void addStaticParticles();

	// perform collision detection using ID map
	void merge();
};


#endif
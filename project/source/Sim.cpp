#include "Sim.hpp"
#include "ShaderPaths.hpp"
#include <atlas/core/Log.hpp>
#include <atlas/core/Float.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <algorithm>

// how many times to calculate the flow direction per timestep
#define FLOW 4

glm::vec2 Sim::dirS = glm::vec2(0.0f, -1.0f);
glm::vec2 Sim::dirSE = glm::normalize(glm::vec2(2.0f, -3.0f));
glm::vec2 Sim::dirSW = glm::normalize(glm::vec2(-2.0f, -3.0f));

Sim::Sim(GridParams const& gp, ParticleParams const& pp) :
	grid(gp),
	angle(gp.angle),
	g(glm::vec2( 0.0f, -9.8e3f * glm::sin(gp.angle) )),
	maxParticles(pp.maxParticles),
	critMass(pp.critMass),
	minInitialMass(pp.minInitialMass),
	maxInitialMass(pp.maxInitialMass),
	meander(pp.meander),
	maxTau(0.1f)
{
	// initialize particles?
	particles.reserve(maxParticles / 4);
	
	mModel = glm::mat4(1.0f);
	// translate to center
	mModel = glm::translate(glm::mat4(1.0f), { -grid.w / 2, -grid.h / 2, 0.0f });
	// rotate around x-axis (assumes the angle is between 0 and 90 degrees)
	mModel = glm::rotate(mModel, 90.0f - angle, { 1.0f, 0.0f, 0.0f });

	// MUST call all of these, & in order!
	gridSetupGl();
	hmapSetupGl();
	pSetupGl();
	idmapSetupGl();
}

Sim::~Sim()  {
	glDeleteBuffers(1, &gVbo);
	glDeleteBuffers(1, &hmapVbo);
	glDeleteBuffers(1, &bbVbo);
	glDeleteBuffers(1, &pVbo);
}
	
void Sim::updateGeometry(atlas::utils::Time const& t)  { 
	float dt = t.deltaTime;

	float i = glm::linearRand<float>(0.0f, 1.0f);
	if (i < 0.2f)
		generate();

	// update moving particles (add trails to height/ID map)
	updateMovingParticles(dt);

	// collision detection (merging)
	if (flags.MERGE)
		merge();

	cleanupParticles();
	//INFO_LOG("particles: " + std::to_string(particles.size()));

	// add particles to height/ID map
	addStaticParticles();

	// smooth & erode operations on height map
	if (flags.SMOOTH)
		grid.smooth();

	if (flags.ERODE)
		grid.erode();
}

void Sim::renderGeometry(atlas::math::Matrix4 projection, atlas::math::Matrix4 view)  { 
	auto mvp = projection * view * mModel;

	// render grid points (temp)
	/*mShaders[0]->enableShaders();
	glUniformMatrix4fv(mUniforms["mvp"], 1, GL_FALSE, &mvp[0][0]);

	glBindVertexArray(gVao);
	glDrawArrays(GL_POINTS, 0, m * n);
	mShaders[0]->disableShaders();*/

	// render height map
	if (!flags.showIDMap) {
		mShaders[1]->enableShaders();
		glUniformMatrix4fv(mUniforms["hmap_mvp"], 1, GL_FALSE, &mvp[0][0]);
		glUniform1f(mUniforms["hmap_res"], grid.res);
		glBindVertexArray(hmapVao);
		// UPDATE BUFFERS
		glBindBuffer(GL_ARRAY_BUFFER, hmapVbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, grid.dim * sizeof(float), grid.height.data());
		glVertexAttribDivisor(0, 0);
		glVertexAttribDivisor(1, 1);
		glVertexAttribDivisor(2, 1);
		glDrawArraysInstanced(GL_TRIANGLES, 0, 6, grid.dim);  // draw m * n squares
		mShaders[1]->disableShaders();
	}
	// render ID map
	else {
		mShaders[3]->enableShaders();
		glUniformMatrix4fv(mUniforms["idmap_mvp"], 1, GL_FALSE, &mvp[0][0]);
		glUniform1f(mUniforms["idmap_res"], grid.res);
		glUniform3fv(mUniforms["colors"], 6, &colors[0][0]);
		glBindVertexArray(idmapVao);
		// update buffers
		glBindBuffer(GL_ARRAY_BUFFER, idmapVbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, grid.dim * sizeof(idnum), grid.id.data());
		glVertexAttribDivisor(0, 0);
		glVertexAttribDivisor(1, 1);
		glVertexAttribDivisor(2, 1);
		glDrawArraysInstanced(GL_TRIANGLES, 0, 6, grid.dim);
		mShaders[3]->disableShaders();
	}

	// render particles
	if (flags.showParticles) {
		mShaders[2]->enableShaders();
		glUniformMatrix4fv(mUniforms["particle_mvp"], 1, GL_FALSE, &mvp[0][0]);

		glBindVertexArray(pVao);
		glBindBuffer(GL_ARRAY_BUFFER, pVbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, particles.size() * sizeof(Particle), 
			particles.data());
		glVertexAttribDivisor(0, 0);
		glVertexAttribDivisor(1, 1);
		glVertexAttribDivisor(2, 1);
		glDrawArraysInstanced(GL_TRIANGLES, 0, 6, (GLsizei)particles.size());
		mShaders[2]->disableShaders();
	}

}

void Sim::gridSetupGl() {
	std::string shaderDir = generated::ShaderPaths::getShaderDirectory();
	std::vector<atlas::gl::ShaderInfo> shaders
	{
		{ GL_VERTEX_SHADER, shaderDir + "grid.vs.glsl" },
		{ GL_FRAGMENT_SHADER, shaderDir + "grid.fs.glsl" }
	};
	mShaders.push_back(ShaderPointer(new atlas::gl::Shader));
	mShaders.back()->compileShaders(shaders);
	mShaders.back()->linkShaders();

	GLuint mvp = mShaders.back()->getUniformVariable("mvp");
	mUniforms.insert(UniformKey("mvp", mvp));
	mShaders.back()->disableShaders();

	// fill VBO
	glGenVertexArrays(1, &gVao);
	glBindVertexArray(gVao);
	glEnableVertexAttribArray(0);

	glGenBuffers(1, &gVbo);
	glBindBuffer(GL_ARRAY_BUFFER, gVbo);
	glBufferData(GL_ARRAY_BUFFER, grid.dim * sizeof(glm::vec2), grid.center.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
}

void Sim::hmapSetupGl() {
	std::string shaderDir = generated::ShaderPaths::getShaderDirectory();
	std::vector<atlas::gl::ShaderInfo> shaders
	{
		{ GL_VERTEX_SHADER, shaderDir + "hmap.vs.glsl" },
		{ GL_FRAGMENT_SHADER, shaderDir + "hmap.fs.glsl" }
	};
	mShaders.push_back(ShaderPointer(new atlas::gl::Shader));
	mShaders.back()->compileShaders(shaders);
	mShaders.back()->linkShaders();

	GLuint hmapmvp = mShaders.back()->getUniformVariable("mvp");
	mUniforms.insert(UniformKey("hmap_mvp", hmapmvp));
	GLuint hmapres = mShaders.back()->getUniformVariable("cellSize");
	mUniforms.insert(UniformKey("hmap_res", hmapres));
	mShaders.back()->disableShaders();

	glm::vec2 points[] = {
		{ -0.5f, -0.5f },{ -0.5f, 0.5f },{ 0.5f, 0.5f },
		{ -0.5f, -0.5f },{ 0.5f, 0.5f },{ 0.5f, -0.5f }
	};
	glGenVertexArrays(1, &hmapVao);
	glBindVertexArray(hmapVao);
	glEnableVertexAttribArray(0);

	glGenBuffers(1, &bbVbo);
	glBindBuffer(GL_ARRAY_BUFFER, bbVbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);  // billboard vertices

	// use gVbo for grid centers
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, gVbo);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);  // grid centers

	glGenBuffers(1, &hmapVbo);
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, hmapVbo);
	glBufferData(GL_ARRAY_BUFFER, grid.dim * sizeof(float), NULL, GL_STREAM_DRAW);
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, 0);  // grid heights

}

void Sim::pSetupGl() {
	std::string shaderDir = generated::ShaderPaths::getShaderDirectory();
	std::vector<atlas::gl::ShaderInfo> shaders
	{
		{ GL_VERTEX_SHADER, shaderDir + "particle.vs.glsl" },
		{ GL_FRAGMENT_SHADER, shaderDir + "particle.fs.glsl" }
	};
	mShaders.push_back(ShaderPointer(new atlas::gl::Shader));
	mShaders.back()->compileShaders(shaders);
	mShaders.back()->linkShaders();

	GLuint mvp = mShaders.back()->getUniformVariable("mvp");
	mUniforms.insert(UniformKey("particle_mvp", mvp));
	mShaders.back()->disableShaders();

	// fill VBO
	glGenVertexArrays(1, &pVao);
	glBindVertexArray(pVao);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, bbVbo);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0); // billboard vertices

	glGenBuffers(1, &pVbo);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, pVbo);
	glBufferData(GL_ARRAY_BUFFER, maxParticles * sizeof(Particle), NULL, GL_STREAM_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), 0); // position
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), 
		(void*)(offsetof(Particle, m))); // mass
}

void Sim::idmapSetupGl() {
	std::string shaderDir = generated::ShaderPaths::getShaderDirectory();
	std::vector<atlas::gl::ShaderInfo> shaders
	{
		{ GL_VERTEX_SHADER, shaderDir + "idmap.vs.glsl" },
		{ GL_FRAGMENT_SHADER, shaderDir + "idmap.fs.glsl" }
	};
	mShaders.push_back(ShaderPointer(new atlas::gl::Shader));
	mShaders.back()->compileShaders(shaders);
	mShaders.back()->linkShaders();

	colors[0] = glm::vec3(0.4f);
	colors[1] = { 1.0f, 1.0f, 0.0f };
	colors[2] = { 1.0f, 0.0f, 1.0f };
	colors[3] = { 0.0f, 1.0f, 1.0f };
	colors[4] = { 0.0f, 1.0f, 0.0f };
	colors[5] = { 1.0, 0.5f, 0.3f };
	

	GLuint mvp = mShaders.back()->getUniformVariable("mvp");
	mUniforms.insert(UniformKey("idmap_mvp", mvp));
	GLuint idres = mShaders.back()->getUniformVariable("cellSize");
	mUniforms.insert(UniformKey("idmap_res", idres));
	GLuint idColors = mShaders.back()->getUniformVariable("colors");
	mUniforms.insert(UniformKey("colors", idColors));

	mShaders.back()->disableShaders();

	glGenVertexArrays(1, &idmapVao);
	glBindVertexArray(idmapVao);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, bbVbo);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0); // billboard vertices

	// use gVbo for grid centers
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, gVbo);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);  // grid centers

	glEnableVertexAttribArray(2);
	glGenBuffers(1, &idmapVbo);
	glBindBuffer(GL_ARRAY_BUFFER, idmapVbo);
	glBufferData(GL_ARRAY_BUFFER, grid.dim * sizeof(idnum), NULL, GL_STREAM_DRAW);
	glVertexAttribPointer(2, 1, GL_UNSIGNED_INT, GL_FALSE, 0, 0);  // id values
}

void Sim::generate() {
	if (particles.size() >= maxParticles)
		return;

	Particle p = {};
	activateParticle(p);

	float x = glm::linearRand<float>(0.0f, grid.w);
	float y = glm::linearRand<float>(0.0f, grid.h);
	p.p = { x, y };

	float mean = (maxInitialMass - minInitialMass) / 5 + minInitialMass;
	float mass = glm::clamp( glm::gaussRand<float>(mean, 1.5f),
		minInitialMass, maxInitialMass );
	p.m = mass;

	// initial velocity = 0 and moving = false
	if (p.m < critMass) {
		// generate static shape

	}
	else
		p.moving = true;

	particles.push_back(p);
	plookup[p.id] = &particles.back();
}

void Sim::activateParticle(Particle& p) {
	p.active = true;
	p.id = idGen.getID();
}

void Sim::cleanupParticles() {
	auto oldsize = particles.size();
	particles.erase(std::remove_if(particles.begin(), particles.end(),
		[](const Particle& p) { return !p.active; }), particles.end());
	// erasing invalidates lookup table (raw pointers)
	if (particles.size() != oldsize) {
		for (auto& p : particles)
			plookup[p.id] = &p;
	}
		
}

void Sim::clearParticles() {
	particles.clear();
	plookup.clear();
	donotmerge.clear();
}

void Sim::addStaticParticles() {
	for (auto const& p : particles) {
		if (!p.moving) {
			float rad = std::cbrt(3.0f * glm::one_over_two_pi<float>() * p.m);
			grid.placeHemisphere(p.p, rad, p.id);
		}
	}
}

bool Sim::residual(float tau, float dt) {
	float pr = glm::min(1.0f, 3 * (dt / maxTau) * glm::min(1.0f, tau / maxTau));
	// if generated number < pr
	float i = glm::linearRand<float>(0.0f, 1.0f);
	return (i <= pr);
}

void Sim::updateMovingParticles(float dt) {
	std::vector<Particle> newparticles;
	for (auto& p : particles) {
		// p.moving set in updateMovingParticles(), moveParticle(), generate(), and merge()
		if (!p.active)  // may not have been cleaned up
			continue;
		if (p.m < critMass) {
			p.moving = false;
			p.v = glm::vec2(0.0f);
			continue;
		}
		// leave residual particle?
		if (flags.RESIDU) {
			if (residual(p.tau, dt)) {
				if (particles.size() + newparticles.size() < maxParticles) {
					Particle p1 = {};
					activateParticle(p1);
					// add parent ID and child ID to do-not-merge table
					donotmerge.insert({ p.id, p1.id });
					p1.p = p.p;
					float a = glm::linearRand<float>(0.1f, 0.3f);
					float m1 = glm::min(critMass, a * p.m);
					p1.m = m1;
					p.m -= m1;

					newparticles.push_back(p1);
				}
				p.tau = 0.0f;
			}
			else
				p.tau += dt;
		}

		moveParticle(p, dt);
	}
	
	// add residual particles
	auto oldsize = particles.size();
	particles.insert(particles.end(), newparticles.begin(), newparticles.end());
	for (auto i = oldsize; i < particles.size(); i++)
		plookup[particles[i].id] = &particles[i];
}

void Sim::moveParticle(Particle& p, float dt) {
	dt /= FLOW;

	for (int i = 0; i < FLOW; i++) {
		// get current grid cell based on position
		long c = grid.index(p.p);
		int a = c % grid.m;
		int b = c / grid.m;

		// work in regions of 3x3 cells - get new direction (unit vector)
		Direction dir;
		glm::vec2 w;
		
		Region regionS = grid.region3x3(a, b + 3);
		Region regionSW = grid.region3x3(a - 2, b + 3);
		Region regionSE = grid.region3x3(a + 2, b + 3);

		if ((regionS.height == 0.0f) && (regionSE.height == 0.0f)
			&& (regionSW.height == 0.0f)) {
			if (regionS.affinity > regionSE.affinity) {
				dir = regionS.affinity > regionSW.affinity ? Direction::S : Direction::SW;
			}
			else {
				dir = regionSE.affinity > regionSW.affinity ? Direction::SE : Direction::SW;
			}
		}
		else if ((regionS.height >= regionSE.height) && (regionS.height >= regionSW.height))
			dir = Direction::S;
		else if (regionSE.height > regionSW.height)
			dir = Direction::SE;
		else
			dir = Direction::SW;

		if (dir == Direction::S) {
			// meander a little bit
			float omega = glm::linearRand<float>(-meander, meander);
			w = glm::rotate(dirS, omega);
		}
		else if (dir == Direction::SE)
			w = dirSE;
		else
			w = dirSW;
		// TODO correct w back towards center depending on velocity?

		// semi-implicit Euler integration
		glm::vec2 acc, v, oldp;
		float s;
		acc = ((p.m - critMass) / p.m) * g;
		s = glm::length(p.v + acc * dt);
		v = s * w;
		p.moving = !(atlas::core::isZero(s));
		p.v = v;
		// ok to update because acc doesn't depend on particle data, only heightmap
		oldp = p.p;
		p.p = p.p + v * dt;

		float rad = std::cbrt(3.0f * glm::one_over_two_pi<float>() * p.m);

		// check if updated position left a gap
		if (glm::length(p.p - oldp) > 1.5f * grid.res) {
			// pre-update height map (linear interp between previous & current position)
			// interpolated coords not aligned with grid but w/e
			glm::vec2 incr = 1.5f * grid.res * glm::normalize(p.p - oldp);

			// y comparison: assumes all movement is downwards (decreasing y-coord)
			for (glm::vec2 lerp = oldp + incr; (lerp.y > p.p.y) && (lerp.y >= 0); lerp += incr)
				grid.placeHemisphere(lerp, rad, p.id);
		}

		grid.placeHemisphere(p.p, rad, p.id);

		// check if p went off the grid
		if ((p.p.y < 0) || (p.p.x < 0) || (p.p.x > grid.w)) {
			p.active = false;
			// remove from hashmap
			plookup.erase(p.id);
			donotmerge.erase(p.id);
			return;
		}
	}
}

void Sim::merge() {
	for (int j = 0; j < grid.n - 1; j++)
		for (int i = 0; i < grid.m - 1; i++) {
			long k = grid.ROWSAFE(i, j);
			auto id1 = grid.id[k];
			if (id1 == 0)
				continue;
			
			int indices[2] = { grid.ROWSAFE(i + 1, j), grid.ROWSAFE(i, j + 1) };
			for (int A = 0; A < 2; A++) {
				id1 = grid.id[k];
				auto id2 = grid.id[indices[A]];
				if ((id2 != 0U) && (id1 != id2)) {
					// get particles (if they exist) from plookup
					// if both particles exist, make new particle
					bool exist1 = plookup.find(id1) != plookup.end();
					bool exist2 = plookup.find(id2) != plookup.end();
					if (exist1 && exist2) {
						Particle* p1 = plookup[id1];
						Particle* p2 = plookup[id2];
						
						if ((p1 < &particles.front()) || (p1 > &particles.back())
							|| (p2 < &particles.front()) || (p2 > &particles.back())) {
							//INFO_LOG("invalid particle lookup (data race?)\nreturning...");
							break;
						}
						if (flags.RESIDU) {
							// check do-not-merge table
							// if p1 is a child of p2 or vice versa, do not merge
							if (donotmerge.find(id1) != donotmerge.end()) {
								auto valueset = donotmerge.equal_range(id1);
								auto result = std::find_if(valueset.first, valueset.second,
									[id2](std::pair<idnum, idnum> ids) { return ids.second == id2; });
								if (result != valueset.second)
									continue;  // no merge
							}
							if (donotmerge.find(id2) != donotmerge.end()) {
								auto valueset = donotmerge.equal_range(id2);
								auto result = std::find_if(valueset.first, valueset.second,
									[id1](std::pair<idnum, idnum> ids) { return ids.second == id1; });
								if (result != valueset.second)
									continue;  // no merge
							}
						}

						// create new particle to replace both p1 and p2, with a new ID
						// could just overwrite one of them, but this is more readable
						Particle p3 = {};
						Particle& lower = p1->p.y < p2->p.y ? *p1 : *p2;
						p3.m = p1->m + p2->m;
						p3.p = lower.p;

						// velocity = sum of momentum / total mass
						//p3.moving = p1->moving || p2->moving;
						p3.v = (p1->m * p1->v + p2->m * p2->v) / p3.m;
						p3.moving = glm::length(p3.v) > 0;
						activateParticle(p3);
						particles.push_back(p3);
						plookup[p3.id] = &particles.back();
						idnum id3 = p3.id;

						// 'jumping merge' problem
						// place a smaller hemisphere between the original positions?
						float radius = std::cbrt(3.0f * glm::one_over_two_pi<float>() *
							p3.m);
						/*glm::vec2 middle = glm::mix(p1->p, p2->p, 0.5f);
						grid.placeHemisphere(middle, 0.75f * radius, id3);*/

						// immediately update grid
						grid.placeHemisphere(p3.p, radius, p3.id);

						// deactivate original particles
						plookup.erase(id1);
						plookup.erase(id2);
						if (flags.RESIDU) {
							if (donotmerge.find(id1) != donotmerge.end()) {
								// copy all the matches to a temp datastructure
								auto valueset = donotmerge.equal_range(id1);
								auto temp = std::vector<std::pair<idnum, idnum> >(valueset.first, valueset.second);
								for (auto const& v : temp)
									donotmerge.insert({ id3, v.second });
							}
							if (donotmerge.find(id2) != donotmerge.end()) {
								auto valueset = donotmerge.equal_range(id2);
								auto temp = std::vector<std::pair<idnum, idnum> >(valueset.first, valueset.second);
								for (auto const& v : temp)
									donotmerge.insert({ id3, v.second });
							}
							donotmerge.erase(id1);
							donotmerge.erase(id2);
						}
						p1->active = false;
						p2->active = false;  // just let cleanup function take care of it

						/*msg = "merge particles to: ";
						msg += "(" + std::to_string(p3.p.x) + ", " + std::to_string(p3.p.y)
							+ ") " + std::to_string(p3.m);
						INFO_LOG(msg);*/

						// update id map
						grid.floodFill(k, id3, id1);
						grid.floodFill(k, id3, id2);
						
					}

					// case where one particle exists in an orphaned ID region
					// TODO instantaneously move particle to align w/ bottom of ID region
					// add mass back in?
					// still lets residual particles disappear
					// also static particles within a large region still contribute mass to it
					else if (exist1) {
						grid.floodFill(k, id1, id2);
					}
					else if (exist2) {
						grid.floodFill(k, id2, id1);
					}
					// if neither particle exists anymore (both are waterflows)
					else if (!exist1 && !exist2) {
						// recolor one of them
						grid.floodFill(k, id1, id2);
					}
				}
			}
		}
}

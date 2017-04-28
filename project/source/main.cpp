#include <atlas/utils/Application.hpp>
#include "AnimationScene.hpp"
#include <atlas/core/Macros.hpp>
#include <atlas/core/Log.hpp>

int main()  {
	APPLICATION.createWindow(1000, 600, "OpenGL 3");
	// set simulation parameters
	float fps = 160.0f;

	float angle = 90.0f;
	float res = 0.5f;  // mm
	int width = 1000;  // number of cells
	int height = 600;
	int numParticles = 700;
	float critMass = 20.f;  // mg
	float maxMass = 100.f;
	float minMass = 2 * pow(res, 3);  // rough approx. of  r > res
	float meanderAngle = 0.3f;

	bool smoothing = true;
	bool erode = true;
	bool leaveResidual = true;
	bool merging = true;
	bool drawParticles = false;
	bool drawIDMap = false;

	GridParams grid = { angle, res, width, height };
	ParticleParams p = { numParticles, critMass, minMass, maxMass, meanderAngle };
	DemoFlags flags = { erode, smoothing, leaveResidual, merging, drawParticles, drawIDMap };

	APPLICATION.addScene(new AnimationScene(fps, flags, grid, p));
	// check for newer OpenGL features
	/*if (GLEW_VERSION_4_3)
		INFO_LOG("OpenGL 4.3 is supported");
	if (GLEW_ARB_compute_shader)
		INFO_LOG("ARB Compute Shaders are supported");
	if (GLEW_ARB_shader_storage_buffer_object)
		INFO_LOG("ARB Shader Storage Buffer Objects are supported");*/
	APPLICATION.runApplication();
	return 0;
}
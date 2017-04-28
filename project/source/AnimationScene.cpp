#include "AnimationScene.hpp"
#include <atlas/core/GLFW.hpp>
#include <atlas/core/Macros.hpp>
#include <atlas/core/Log.hpp>

AnimationScene::AnimationScene(float fps, DemoFlags const& initialFlags, GridParams const& g, ParticleParams const& p) :
	updateCounter(fps),
	grid(g, p),
	gWidth(g.m * g.res / 2),
	gHeight(g.n * g.res / 2),
	measurementCounter(2.0)
{
	grid.flags = initialFlags;

	gDiam = glm::max(gWidth, gHeight);
	gAspect = gWidth / gHeight;

	mProjection = glm::ortho(-gDiam, gDiam, -gDiam, gDiam);
	mView = glm::mat4(1.0f);
	
	glClearColor(0.21f, 0.21f, 0.2f, 1.0f);
	//glEnable(GL_DEPTH_TEST);
}

AnimationScene::~AnimationScene()  { }

void AnimationScene::screenResizeEvent(int width, int height) {
	glViewport(0, 0, width, height);
	float aspect = (float)width / height;
	// window taller than object
	if (aspect < gAspect)
		mProjection = glm::ortho(-gWidth, gWidth, 
			-gWidth / aspect, gWidth / aspect);
	else
		mProjection = glm::ortho(-gHeight * aspect, gHeight * aspect, 
			-gHeight, gHeight);
	//mProjection = glm::perspective(glm::radians(45.0), (double)width / height, 1.0, 1000.0);
}

void AnimationScene::renderScene()  {
	glClear(GL_COLOR_BUFFER_BIT);
	grid.renderGeometry(mProjection, mView);
}


void AnimationScene::updateScene(double time)  {
	mTime.deltaTime = (float)time - mTime.currentTime;
	mTime.totalTime += mTime.deltaTime;
	mTime.currentTime = (float)time;

	if (updateCounter.isFPS(mTime)) {  // modifies mTime
		grid.updateGeometry(mTime);
		measurementCounter.update(time);
	}
}

void AnimationScene::keyPressEvent(int key, int scancode, int action, int mods) {
	UNUSED(scancode);
	UNUSED(mods);

	if (action == GLFW_PRESS)
	{
		std::string msg;
		switch (key)
		{
		case GLFW_KEY_R:
			grid.flags.RESIDU = !grid.flags.RESIDU;
			msg = "residual particles ";
			msg += (grid.flags.RESIDU ? "enabled" : "disabled");
			INFO_LOG(msg);
			break;

		case GLFW_KEY_S:
			grid.flags.SMOOTH = !grid.flags.SMOOTH;
			msg = "smoothing ";
			msg += (grid.flags.SMOOTH ? "enabled" : "disabled");
			INFO_LOG(msg);
			break;

		case GLFW_KEY_E:
			grid.flags.ERODE = !grid.flags.ERODE;
			msg = "erosion ";
			msg += (grid.flags.ERODE ? "enabled" : "disabled");
			INFO_LOG(msg);
			break;

		case GLFW_KEY_M:
			grid.flags.MERGE = !grid.flags.MERGE;
			msg = "merging ";
			msg += (grid.flags.MERGE ? "enabled" : "disabled");
			INFO_LOG(msg);
			break;

		case GLFW_KEY_P:
			grid.flags.showParticles = !grid.flags.showParticles;
			break;

		case GLFW_KEY_X:
			grid.clearParticles();
			INFO_LOG("=== particles cleared ===");
			break;

		case GLFW_KEY_I:
			grid.flags.showIDMap = !grid.flags.showIDMap;
			break;

		default:
			break;
		}
	}
}

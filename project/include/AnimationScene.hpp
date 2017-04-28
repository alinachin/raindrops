#ifndef INCLUDE_ANIMATION_SCENE
#define INCLUDE_ANIMATION_SCENE

#pragma once

#include <atlas/utils/Scene.hpp>
#include <atlas/utils/FPSCounter.hpp>
#include "Sim.hpp"
#include "TimeCounter.hpp"

class AnimationScene : public atlas::utils::Scene  {
public:
	AnimationScene(float fps, DemoFlags const& initialFlags, GridParams const& g, ParticleParams const& p);
	~AnimationScene();

	void screenResizeEvent(int width, int height) override;
	
	void renderScene() override;
	void updateScene(double time) override;

	void keyPressEvent(int key, int scancode, int action, int mods) override;
	
private:
	atlas::utils::FPSCounter updateCounter;
	TimeCounter measurementCounter;
	Sim grid;
	float gWidth, gHeight, gDiam, gAspect;
};

#endif
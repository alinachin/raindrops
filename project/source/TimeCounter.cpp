#include "TimeCounter.hpp"
#include <atlas/core/Log.hpp>

TimeCounter::TimeCounter(float logPeriod) :
	logPeriod(logPeriod),
	lastTime(0.0),
	frameCounter(0)
{ }

void TimeCounter::update(double currentTime)
{
	if (lastTime == 0.0)
		lastTime = currentTime;
	frameCounter++;
	if (currentTime - lastTime >= logPeriod)
		logAverageTime(currentTime);
}

void TimeCounter::logAverageTime(double currentTime)
{
	double period_ms = (currentTime - lastTime) * 1000.;
	double avgMsPerFrame = period_ms / frameCounter;
	std::string msg = std::to_string(avgMsPerFrame);
	msg.resize(8);
	INFO_LOG("Timing: " + msg + " ms");

	frameCounter = 0;
	lastTime = currentTime;
}


#ifndef INCLUDE_TIME_COUNTER
#define INCLUDE_TIME_COUNTER

#pragma once

class TimeCounter {
public:
	TimeCounter(float logPeriod);
	void update(double currentTime);
private:
	float logPeriod;
	double lastTime;
	int frameCounter;
	void logAverageTime(double currentTime);
};

#endif
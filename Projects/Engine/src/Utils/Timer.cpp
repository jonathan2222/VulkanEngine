#include "stdafx.h"
#include "Timer.h"

ym::Timer::Timer()
{
	start();
}

ym::Timer::~Timer()
{

}

void ym::Timer::start()
{
	m_startTime = std::chrono::high_resolution_clock::now();
}

float ym::Timer::stop()
{
	m_endTime = std::chrono::high_resolution_clock::now();
	float dt = std::chrono::duration<float>(m_endTime - m_startTime).count();
	return dt;
}
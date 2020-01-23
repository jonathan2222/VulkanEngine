#include "stdafx.h"
#include "InstrumentationTimer.h"

ym::InstrumentationTimer::InstrumentationTimer(const std::string& name, bool active) : m_name(name), m_active(active)
{
	start();
}

ym::InstrumentationTimer::~InstrumentationTimer()
{
	stop();
}

void ym::InstrumentationTimer::start()
{
	if (m_active)
		m_startTime = std::chrono::high_resolution_clock::now();
}

void ym::InstrumentationTimer::stop()
{
	if (m_active)
	{
		m_endTime = std::chrono::high_resolution_clock::now();
		long long start = std::chrono::time_point_cast<std::chrono::microseconds>(m_startTime).time_since_epoch().count();
		long long end = std::chrono::time_point_cast<std::chrono::microseconds>(m_endTime).time_since_epoch().count();

		Instrumentation::get().write({ m_name, start, end });
	}
}

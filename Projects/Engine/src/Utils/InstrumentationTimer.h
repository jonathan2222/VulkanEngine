#pragma once

#include <chrono>
#include <string>

#include "Instrumentation.h"

// Code from: https://www.youtube.com/watch?v=xlAH4dbMVnU

#ifdef YAMI_DEBUG
	#define YM_PROFILER_BEGIN_SESSION(name, fileName) Instrumentation::get().beginSession(name, fileName)
	#define YM_PROFILER_END_SESSION() Instrumentation::get().endSession()
	#define YM_PROFILER_SCOPE(name) InstrumentationTimer instrumentationTimer##__LINE__(name)
	#define YM_PROFILER_FUNCTION() YM_PROFILER_SCOPE(__FUNCTION__)

	#define YM_PROFILER_RENDERING_BEGIN_SESSION(name, fileName) {if(!ym::Instrumentation::g_runRenderingProfiling) { Instrumentation::get().beginSession(name, fileName); ym::Instrumentation::g_runRenderingProfiling = true; }}
	#define YM_PROFILER_RENDERING_END_SESSION() {if(ym::Instrumentation::g_runRenderingProfiling) { Instrumentation::get().endSession(); ym::Instrumentation::g_runRenderingProfiling = false; } }
	#define YM_PROFILER_RENDERING_SCOPE(name) InstrumentationTimer instrumentationTimerRendering##__LINE__(name, ym::Instrumentation::g_runRenderingProfiling)
	#define YM_PROFILER_RENDERING_FUNCTION() YM_PROFILER_RENDERING_SCOPE(__FUNCTION__)
#else
#define YM_PROFILER_BEGIN_SESSION(name, fileName)
#define YM_PROFILER_END_SESSION()
#define YM_PROFILER_SCOPE(name)
#define YM_PROFILER_FUNCTION()

#define YM_PROFILER_RENDERING_BEGIN_SESSION(name, fileName)
#define YM_PROFILER_RENDERING_END_SESSION()
#define YM_PROFILER_RENDERING_SCOPE(name)
#define YM_PROFILER_RENDERING_FUNCTION()
#endif

namespace ym
{
	class InstrumentationTimer
	{
	public:
		InstrumentationTimer(const std::string& name, bool active = true);
		~InstrumentationTimer();

		void start();
		void stop();

	private:
		std::chrono::time_point<std::chrono::steady_clock> m_startTime;
		std::chrono::time_point<std::chrono::steady_clock> m_endTime;
		std::string m_name;
		bool m_active;
	};
}
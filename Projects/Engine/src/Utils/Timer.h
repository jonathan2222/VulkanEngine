#pragma once

#include <chrono>

namespace ym
{
	class Timer
	{
	public:
		Timer();
		~Timer();

		void start();
		float stop();

	private:
		std::chrono::time_point<std::chrono::steady_clock> m_startTime;
		std::chrono::time_point<std::chrono::steady_clock> m_endTime;
	};
}
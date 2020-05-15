#pragma once

#include <cstdint>

#define MAX_CIRCULAR_BUFFER_SIZE 5 // In Seconds

namespace ym
{
	class CircularBuffer
	{
	public:
		uint64_t size{ 0 };
		uint64_t endIndex{ 0 };
		float* data{ nullptr }; // This functions like a ring buffer.

		void init(uint64_t sampleRate);
		void clearAll();
		void clear();
		void destroy();
		void setDelay(double seconds);
		void setDelayInSamples(int64_t samples);

		/*
			Get oldest sample.
		*/
		float get() const;

		/*
			Add new sample to the start.
		*/
		void add(float v);

	private:
		void next();

		uint64_t start{ 0 };
		uint64_t end{ 2 };
		uint64_t sampleRate{ 0 };
	};
}
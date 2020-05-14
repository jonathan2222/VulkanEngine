#pragma once

#include <cstdint>
#include <utility>

namespace ym
{
	struct SoundData;
	class Filter
	{
	public:
		virtual void init(uint64_t sampleRate) = 0;
		virtual void destroy() = 0;
		/*
			Called before processing current block of data.
		*/

		virtual void begin() = 0;
		/*
			Called wen one pair of data should be processed.
		*/

		virtual std::pair<float, float> process(SoundData* data, float left, float right) = 0;

		virtual std::string getName() const = 0;
	};
}
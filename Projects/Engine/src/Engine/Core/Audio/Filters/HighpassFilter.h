#pragma once

#include "Filter.h"
#include "../CircularBuffer.h"

namespace ym
{
	class HighpassFilter : public Filter
	{
	public:
		void init(uint64_t sampleRate) override;
		void destroy() override;
		void begin() override;
		std::pair<float, float> process(SoundData* data, float left, float right) override;
		std::string getName() const override;

		void setCutoffFrequency(float cutoffFrequency);
		float getCutoffFrequency() const;
		float getSampleRate() const;

	private:
		CircularBuffer delayBuffer;

		float cutoffFrequency{ 20000.f };
		float sampleRate{ 0.f };
	};
}
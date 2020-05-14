#pragma once

#include "Filter.h"
#include "../CircularBuffer.h"

namespace ym
{
	class EchoFilter : public Filter
	{
	public:
		void init(uint64_t sampleRate) override;
		void destroy() override;
		void begin() override;
		std::pair<float, float> process(SoundData* data, float left, float right) override;
		std::string getName() const override;

		void setDelay(float delay);
		void setGain(float gain);

		float getDelay() const;
		float getGain() const;

	private:
		CircularBuffer delayBuffer;

		float delay{ 0.5f };
		float gain{0.5f};
	};
}
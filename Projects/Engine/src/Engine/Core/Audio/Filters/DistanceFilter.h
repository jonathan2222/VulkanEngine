#pragma once

#include "Filter.h"
#include <glm/glm.hpp>

namespace ym
{
	class DistanceFilter : public Filter
	{
	public:
		void init(uint64_t sampleRate) override;
		void destroy() override;
		void begin() override;
		std::pair<float, float> process(SoundData* data, float left, float right) override;
		std::string getName() const override;
	};
}
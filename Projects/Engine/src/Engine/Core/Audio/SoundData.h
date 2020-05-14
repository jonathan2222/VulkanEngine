#pragma once

#include <glm/glm.hpp>

namespace ym
{
	struct SoundData
	{
		float volume{ 1.f };
		bool loop{ false };
		glm::vec3 sourcePos;
		glm::vec3 receiverPos;
		glm::vec3 receiverLeft;
		glm::vec3 receiverUp;
	};
}
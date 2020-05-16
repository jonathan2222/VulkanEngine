#pragma once

#include <glm/glm.hpp>

namespace ym
{
	struct SoundData
	{
		float masterVolume{1.f};	// Volume for all sounds
		float groupVolume{1.f};		// Volume for effects/streams
		float volume{ 1.f };		// Volume for this sound

		bool loop{ false };
		glm::vec3 sourcePos;
		glm::vec3 receiverPos;
		glm::vec3 receiverLeft;
		glm::vec3 receiverUp;
	};
}
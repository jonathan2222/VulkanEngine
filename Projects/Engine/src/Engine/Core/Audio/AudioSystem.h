#pragma once

#include <FMOD/fmod.hpp>

namespace ym
{
	class Sound;
	class AudioSystem
	{
	public:
		AudioSystem();
		~AudioSystem();

		static AudioSystem* get();

		void init();
		void destory();

		void update();

		Sound* createSound(const std::string& filePath);
		void removeSound(Sound* sound);

	private:
		FMOD::System* system;
		uint32_t version;

		std::vector<Sound*> sounds;
	};
}
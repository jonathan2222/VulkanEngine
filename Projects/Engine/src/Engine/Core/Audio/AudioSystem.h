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

		Sound* createStream(const std::string& filePath);
		void removeStream(Sound* soundStream);

	private:
		FMOD::System* system;
		uint32_t version;

		std::vector<Sound*> sounds;
		std::vector<Sound*> soundStreams;
	};
}
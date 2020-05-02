#pragma once

#include "DR/dr_helper.h"
#include "PortAudio.h"

#define SAMPLE_RATE 44100

namespace ym
{
	class PortAudio;
	class Sound;
	class ChannelGroup;
	class AudioSystem
	{
	public:
		AudioSystem();
		~AudioSystem();

		static AudioSystem* get();

		void init();
		void destroy();

		void update();

		Sound* createSound(const std::string& filePath);
		void removeSound(Sound* sound);

		Sound* createStream(const std::string& filePath);
		void removeStream(Sound* soundStream);

		static void loadFile(SoundHandle* soundHandle, const std::string& filePath);

	private:
		std::vector<Sound*> sounds;
		std::vector<Sound*> soundStreams;
		PortAudio* portAudio;
	};
}
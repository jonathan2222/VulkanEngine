#pragma once

#include "DR/dr_helper.h"

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

		static void loadFile(SoundHandler* soundHandler, const std::string& filePath);
		static uint64_t readPCMFrames(SoundHandler* soundHandler, uint64_t framesToRead, float* outBuffer);
		static uint64_t readPCMFrames(SoundHandler* soundHandler, uint64_t framesToRead, int16_t* outBuffer);

	private:
		std::vector<Sound*> sounds;
		std::vector<Sound*> soundStreams;
		PortAudio* portAudio;
	};
}
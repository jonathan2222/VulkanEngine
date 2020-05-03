#pragma once

#include "PCMFunctions.h"

namespace ym
{
	class PortAudio;
	class ChannelGroup;
	class Sound
	{
	public:
		Sound(PortAudio* portAudioPtr);
		~Sound();

		void init(PCM::UserData* userData, PCM::Func pcmFunction);
		void destroy();

		void play();
		void pause();
		void unpause();
		void stop();

		float getVolume() const;
		void applyVolume(float volumeChange);
		void setVolume(float volume);
		void setLoop(bool state);
		void setDistance(float distance);

		friend class AudioSystem;
	private:
		bool isCreated() const;

	private:
		float volume;
		PaStream* stream;
		PortAudio* portAudioPtr;
		PCM::UserData* userData;
		bool isStreamOn;
	};
}
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

		void setSourcePosition(const glm::vec3& sourcePos);
		void setReceiverPosition(const glm::vec3& receiverPos);
		void setReceiverDir(const glm::vec3& receiverDir);
		void setReceiverLeft(const glm::vec3& receiverLeft);
		void setReceiverUp(const glm::vec3& receiverUp);

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
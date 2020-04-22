#pragma once

#include <FMOD/fmod.hpp>

namespace ym
{
	class Sound
	{
	public:
		Sound(FMOD::System* systemPtr);
		~Sound();

		void init();
		void destory();

		void play();
		void pause();
		void unpause();
		void stop();

		float getVolume() const;
		void applyVolume(float volumeChange);
		void setVolume(float volume);
		void setLoop(bool state);

		friend class AudioSystem;
	private:
		bool isCreated() const;
		bool isChannelCreate() const;

	private:
		FMOD::Sound* sound;
		FMOD::Channel* channel;
		FMOD::System* systemPtr;
		float volume;
	};
}
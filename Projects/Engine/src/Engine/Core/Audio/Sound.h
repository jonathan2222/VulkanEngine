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

		friend class AudioSystem;
	private:
		bool isCreated() const;

	private:
		FMOD::Sound* sound;
		FMOD::Channel* channel;
		FMOD::System* systemPtr;
	};
}
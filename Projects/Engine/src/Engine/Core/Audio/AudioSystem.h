#pragma once

#include "PCMFunctions.h"

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

		void setMasterVolume(float volume);
		void setStreamVolume(float volume);
		void setEffectsVolume(float volume);

		// Assume the sound file is in stereo (Two channels). (This can be fixed in the PCMFunctions.h when processing the data!)
		Sound* createSound(const std::string& filePath);
		void removeSound(Sound* sound);

		/*
			Will stream the sound when playing and loop it when reaching the end.
			Assume the sound file is in stereo (Two channels). (This can be fixed in the PCMFunctions.h when processing the data!)
			TODO: Fix memory leaks when using this! (Have to do with the dr library for loading from file)
		*/
		Sound* createStream(const std::string& filePath);
		void removeStream(Sound* soundStream);

		static void loadStreamFile(SoundHandle* soundHandle, const std::string& filePath);
		static void loadEffectFile(SoundHandle* soundHandle, const std::string& filePath, PaSampleFormat format);

		// Debug
		void drawAudioSettings();

	private:
		void drawSoundSettings(Sound* sound);

		std::vector<Sound*> sounds;
		std::vector<Sound*> soundStreams;
		PortAudio* portAudio;

		float effectsVolume{ 1.f };
		float streamVolume{ 1.f };
		float masterVolume{ 1.f };
	};
}
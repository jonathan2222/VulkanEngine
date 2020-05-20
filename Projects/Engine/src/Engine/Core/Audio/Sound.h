#pragma once

#include "PCMFunctions.h"
#include "Filters/Filter.h"

namespace ym
{
	class Camera;
	class PortAudio;
	class ChannelGroup;
	class Sound
	{
	public:
		Sound(PortAudio* portAudioPtr);
		~Sound();

		void init(PCM::UserData* userData);
		void destroy();

		void play();
		void pause();
		void unpause();
		void stop();

		void setName(const std::string& name);
		std::string getName() const;
		glm::vec3 getPosition() const;

		float getVolume() const;
		void applyVolume(float volumeChange);
		void setVolume(float volume);
		void setGroupVolume(float volume);
		void setMasterVolume(float volume);
		void setLoop(bool state);

		void addFilter(Filter* filter);
		std::vector<Filter*>& getFilters();
		
		void setSourcePosition(const glm::vec3& sourcePos);
		void setReceiverPosition(const glm::vec3& receiverPos);
		void setReceiverDir(const glm::vec3& receiverDir);
		void setReceiverLeft(const glm::vec3& receiverLeft);
		void setReceiverUp(const glm::vec3& receiverUp);
		void setReceiver(Camera* camera);
		
		/*
		void setCutoffFrequency(float fc);
		float getCutoffFrequency() const;
		uint32_t getSampleRate() const;

		void setEchoData(float delay, float gain);
		*/
		

		friend class AudioSystem;
	private:
		bool isCreated() const;

	private:
		float volume;
		PaStream* stream;
		PortAudio* portAudioPtr;
		PCM::UserData* userData;
		bool isStreamOn;
		std::string name{"NO_NAME"};
	};
}
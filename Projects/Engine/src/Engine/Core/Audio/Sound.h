#pragma once


namespace ym
{
	class ChannelGroup;
	class Sound
	{
	public:
		Sound();
		~Sound();

		void init();
		void destroy();

		void play();
		void pause();
		void unpause();
		void stop();

		void setChannelGroup(ChannelGroup* channelGroup);

		float getVolume() const;
		void applyVolume(float volumeChange);
		void setVolume(float volume);
		void setLoop(bool state);

		friend class AudioSystem;
	private:
		bool isCreated() const;
		bool isChannelValid();

	private:
		float volume;
	};
}
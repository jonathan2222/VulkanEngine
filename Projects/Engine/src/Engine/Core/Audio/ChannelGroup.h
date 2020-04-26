#pragma once

#include <FMOD/fmod.hpp>

namespace ym
{
	class ChannelGroup
	{
	public:
		ChannelGroup(FMOD::System* systemPtr);
		~ChannelGroup();

		void init(const std::string& name);
		void destroy();

	private:
		bool isCreated() const;

		friend class Sound;
	private:
		FMOD::System* systemPtr;
		FMOD::ChannelGroup* channelGroup;
		std::string name;
	};
}
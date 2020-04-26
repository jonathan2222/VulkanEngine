#include "stdafx.h"
#include "ChannelGroup.h"

#include <FMOD/fmod_errors.h>

ym::ChannelGroup::ChannelGroup(FMOD::System* systemPtr) : systemPtr(systemPtr), channelGroup(nullptr), name("UNKNOWN_NAME")
{
}

ym::ChannelGroup::~ChannelGroup()
{
	destroy();
}

void ym::ChannelGroup::init(const std::string& name)
{
	if (!isCreated())
	{
		this->name = name;
		FMOD_CHECK(this->systemPtr->createChannelGroup(name.c_str(), &this->channelGroup), "Failed to create channel group \"{}\"!", name.c_str());
	}
}

void ym::ChannelGroup::destroy()
{
	if (isCreated())
	{
		this->channelGroup->release();
		this->channelGroup = nullptr;
		this->name = "UNKNOWN_NAME";
	}
}

bool ym::ChannelGroup::isCreated() const
{
	return this->channelGroup != nullptr;
}

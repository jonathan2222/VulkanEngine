#include "stdafx.h"
#include "Sound.h"

#include <FMOD/fmod_errors.h>

#include "ChannelGroup.h"

ym::Sound::Sound(FMOD::System* systemPtr) : sound(nullptr), channel(nullptr), systemPtr(systemPtr), volume(0.0f)
{
}

ym::Sound::~Sound()
{
	destroy();
}

void ym::Sound::init()
{
	
}

void ym::Sound::destroy()
{
	if (isCreated())
	{
		FMOD_CHECK(this->sound->release(), "Failed to release sound!");
		this->sound = nullptr;
		this->channel = nullptr;
		this->systemPtr = nullptr;
	}
}

void ym::Sound::play()
{
	FMOD_CHECK(this->systemPtr->playSound(this->sound, 0, false, &this->channel), "Failed to play sound!");
	FMOD_CHECK(this->channel->getVolume(&this->volume), "Failed to get sound volume!");
}

void ym::Sound::pause()
{
	if (isChannelValid())
	{
		FMOD_CHECK(this->channel->setPaused(true), "Failed to pause sound!");
	}
}

void ym::Sound::unpause()
{
	if (isChannelValid())
	{
		FMOD_CHECK(this->channel->setPaused(false), "Failed to unpause sound!");
	}
}

void ym::Sound::stop()
{
	if (isChannelValid())
	{
		FMOD_CHECK(this->channel->stop(), "Failed to stop sound!");
		this->channel = nullptr;
	}
}

void ym::Sound::setChannelGroup(ChannelGroup* channelGroup)
{
	if (isChannelValid())
	{
		FMOD_CHECK(this->channel->setChannelGroup(channelGroup->channelGroup), "Failed to set channel group \"{}\" to sound!", channelGroup->name.c_str());
	}
}

float ym::Sound::getVolume() const
{
	return this->volume;
}

void ym::Sound::applyVolume(float volumeChange)
{
	this->volume += volumeChange;
	setVolume(this->volume);
}

void ym::Sound::setVolume(float volume)
{
	this->volume = volume;
	if (isChannelValid())
	{
		FMOD_CHECK(this->channel->setVolume(volume), "Failed to set sound volume!");
	}
}

void ym::Sound::setLoop(bool state)
{
	if (state)
	{
		FMOD_CHECK(sound->setMode(FMOD_LOOP_NORMAL), "Failed to set sound mode!");
	}
	else
	{
		FMOD_CHECK(sound->setMode(FMOD_LOOP_OFF), "Failed to set sound mode!");
	}
}

bool ym::Sound::isCreated() const
{
	return this->sound != nullptr;
}

bool ym::Sound::isChannelValid()
{
	// Use the isPlaying function to check if the channel handel is invalid or not.
	if (this->channel != nullptr)
	{
		bool isPlaying;
		FMOD_RESULT result = this->channel->isPlaying(&isPlaying);
		if (result == FMOD_ERR_INVALID_HANDLE) this->channel = nullptr;
		return this->channel != nullptr;
	}
	else
		return this->channel != nullptr;
}

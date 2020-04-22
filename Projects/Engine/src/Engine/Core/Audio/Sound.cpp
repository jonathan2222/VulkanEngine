#include "stdafx.h"
#include "Sound.h"

#include <FMOD/fmod_errors.h>

ym::Sound::Sound(FMOD::System* systemPtr) : sound(nullptr), channel(nullptr), systemPtr(systemPtr), volume(0.0f)
{
}

ym::Sound::~Sound()
{
	destory();
}

void ym::Sound::init()
{
	
}

void ym::Sound::destory()
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
	if (isChannelCreate())
		unpause();
	else
	{
		FMOD_CHECK(this->systemPtr->playSound(this->sound, 0, false, &this->channel), "Failed to play sound!");
		FMOD_CHECK(this->channel->getVolume(&this->volume), "Failed to get sound volume!");
	}
}

void ym::Sound::pause()
{
	if (isChannelCreate())
	{
		FMOD_CHECK(this->channel->setPaused(true), "Failed to pause sound!");
	}
}

void ym::Sound::unpause()
{
	if (isChannelCreate())
	{
		FMOD_CHECK(this->channel->setPaused(false), "Failed to unpause sound!");
	}
}

void ym::Sound::stop()
{
	if (isChannelCreate())
	{
		FMOD_CHECK(this->channel->stop(), "Failed to stop sound!");
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
	if (isChannelCreate())
	{
		this->volume = volume;
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

bool ym::Sound::isChannelCreate() const
{
	return this->channel != nullptr;
}

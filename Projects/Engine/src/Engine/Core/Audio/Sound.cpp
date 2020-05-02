#include "stdafx.h"
#include "Sound.h"

ym::Sound::Sound() : volume(0.0f)
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
		
	}
}

void ym::Sound::play()
{
}

void ym::Sound::pause()
{
	if (isChannelValid())
	{
	}
}

void ym::Sound::unpause()
{
	if (isChannelValid())
	{
	}
}

void ym::Sound::stop()
{
	if (isChannelValid())
	{
	}
}

void ym::Sound::setChannelGroup(ChannelGroup* channelGroup)
{
	if (isChannelValid())
	{
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
	}
}

void ym::Sound::setLoop(bool state)
{
	if (state)
	{
	}
	else
	{
	}
}

bool ym::Sound::isCreated() const
{
	return false;
}

bool ym::Sound::isChannelValid()
{
	return true;
}

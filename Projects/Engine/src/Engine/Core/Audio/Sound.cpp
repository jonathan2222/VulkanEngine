#include "stdafx.h"
#include "Sound.h"

#include <FMOD/fmod_errors.h>

ym::Sound::Sound(FMOD::System* systemPtr) : sound(nullptr), channel(nullptr), systemPtr(systemPtr)
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
	FMOD_CHECK(this->systemPtr->playSound(this->sound, 0, false, &this->channel), "Failed to play sound!");
}

bool ym::Sound::isCreated() const
{
	return this->sound != nullptr;
}

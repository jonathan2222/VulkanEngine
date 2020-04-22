#include "stdafx.h"
#include "AudioSystem.h"

#include <FMOD/fmod_errors.h>
#include "Sound.h"

ym::AudioSystem::AudioSystem() : system(nullptr)
{
}

ym::AudioSystem::~AudioSystem()
{
}

ym::AudioSystem* ym::AudioSystem::get()
{
	static AudioSystem audioSystem;
	return &audioSystem;
}

void ym::AudioSystem::init()
{
	FMOD_CHECK(FMOD::System_Create(&this->system), "Could not create FMOD system!");

	FMOD_CHECK(this->system->getVersion(&this->version), "Could not fetch FMOD version from system!");
	YM_ASSERT(this->version >= FMOD_VERSION, "FMOD lib version {08:x} does not match header version {08:x}.", version, FMOD_VERSION);

	int32_t maxChannels = 512;
	FMOD_CHECK(this->system->init(maxChannels, FMOD_INIT_NORMAL, nullptr), "Could not initialize FMOD system!");

	YM_LOG_INFO("Initialized audio system with max {} channels.", maxChannels);
}

void ym::AudioSystem::destory()
{
	// Remove all sounds create by the system.
	for (Sound*& sound : this->sounds)
	{
		sound->destory();
		delete sound;
	}
	this->sounds.clear();

	FMOD_CHECK(this->system->close(), "Could not close FMOD system!");
	FMOD_CHECK(this->system->release(), "Could not release FMOD system!");

	YM_LOG_INFO("Destroyed audio system.");
}

void ym::AudioSystem::update()
{
	this->system->update();
}

ym::Sound* ym::AudioSystem::createSound(const std::string& filePath)
{
	Sound* sound = new Sound(this->system);
	FMOD_CHECK(this->system->createSound(filePath.c_str(), FMOD_DEFAULT, 0, &sound->sound), "Failed to create sound {}!", filePath.c_str());
	FMOD_CHECK(sound->sound->setMode(FMOD_LOOP_OFF), "Failed to set sound mode for sound {}!", filePath.c_str());
	this->sounds.push_back(sound);
	return sound;
}

void ym::AudioSystem::removeSound(Sound* sound)
{
	sound->destory();
	std::remove(this->sounds.begin(), this->sounds.end(), sound);
	delete sound;
}

#include "stdafx.h"
#include "AudioSystem.h"

#include <FMOD/fmod_errors.h>
#include "Sound.h"
#include "ChannelGroup.h"

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

void ym::AudioSystem::destroy()
{
	// Remove all sounds create by the system.
	for (Sound*& sound : this->sounds)
	{
		sound->destroy();
		delete sound;
	}
	this->sounds.clear();

	// Remove all soundStreams create by the system.
	for (Sound*& stream : this->soundStreams)
	{
		stream->destroy();
		delete stream;
	}
	this->soundStreams.clear();

	// Remove all channel groups created by the system.
	for (ChannelGroup*& channelGroup : this->channelGroups)
	{
		channelGroup->destroy();
		delete channelGroup;
	}
	this->channelGroups.clear();

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
	sound->init();
	this->sounds.push_back(sound);
	return sound;
}

void ym::AudioSystem::removeSound(Sound* sound)
{
	sound->destroy();
	this->sounds.erase(std::remove(this->sounds.begin(), this->sounds.end(), sound), this->sounds.end());
	delete sound;
}

ym::Sound* ym::AudioSystem::createStream(const std::string& filePath)
{
	Sound* stream = new Sound(this->system);
	FMOD_CHECK(this->system->createStream(filePath.c_str(), FMOD_DEFAULT, nullptr, &stream->sound), "Failed to create sound stream {}!", filePath.c_str());
	FMOD_CHECK(stream->sound->setMode(FMOD_LOOP_NORMAL), "Failed to set sound mode for sound stream {}!", filePath.c_str());
	stream->init();
	this->soundStreams.push_back(stream);
	return stream;
}

void ym::AudioSystem::removeStream(Sound* soundStream)
{
	soundStream->destroy();
	this->soundStreams.erase(std::remove(this->soundStreams.begin(), this->soundStreams.end(), soundStream), this->soundStreams.end());
	delete soundStream;
}

ym::ChannelGroup* ym::AudioSystem::createChannelGroup(const std::string& name)
{
	ChannelGroup* channelGroup = new ChannelGroup(this->system);
	channelGroup->init(name);
	this->channelGroups.push_back(channelGroup);
	return channelGroup;
}

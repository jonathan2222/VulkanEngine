#include "stdafx.h"
#include "AudioSystem.h"

#include "Sound.h"

#define DR_MP3_IMPLEMENTATION
#include "DR/dr_mp3.h"
#define DR_WAV_IMPLEMENTATION
#include "DR/dr_wav.h"

#include "PCMFunctions.h"

ym::AudioSystem::AudioSystem() : portAudio(nullptr)
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
	this->portAudio = new PortAudio();
	this->portAudio->init();
	YM_LOG_INFO("Initialized audio system.");
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

	this->portAudio->destroy();
	SAFE_DELETE(this->portAudio);

	YM_LOG_INFO("Destroyed audio system.");
}

void ym::AudioSystem::update()
{
	
}

ym::Sound* ym::AudioSystem::createSound(const std::string& filePath)
{
	Sound* sound = new Sound(this->portAudio);
	PCM::UserData* userData = new PCM::UserData();
	userData->finished = false;
	userData->sampleFormat = paFloat32;
	loadEffectFile(&userData->handle, filePath, userData->sampleFormat);
	sound->init(userData);
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
	Sound* stream = new Sound(this->portAudio);
	PCM::UserData* userData = new PCM::UserData();
	userData->finished = false;
	userData->sampleFormat = paFloat32;
	userData->loop = true;
	loadStreamFile(&userData->handle, filePath);
	stream->init(userData);
	this->soundStreams.push_back(stream);
	return stream;
}

void ym::AudioSystem::removeStream(Sound* soundStream)
{
	soundStream->destroy();
	this->soundStreams.erase(std::remove(this->soundStreams.begin(), this->soundStreams.end(), soundStream), this->soundStreams.end());
	delete soundStream;
}

void ym::AudioSystem::loadStreamFile(SoundHandle* soundHandle, const std::string& filePath)
{
	soundHandle->asEffect = false;

	// Get file type.
	std::string ext = filePath.substr(filePath.find_last_of(".") + 1);
	if (ext == "mp3") soundHandle->type = SoundHandle::Type::TYPE_MP3;
	if (ext == "wav") soundHandle->type = SoundHandle::Type::TYPE_WAV;

	// Load file.
	switch (soundHandle->type)
	{
	case SoundHandle::Type::TYPE_MP3:
		{
			YM_ASSERT(drmp3_init_file(&soundHandle->mp3, filePath.c_str(), NULL), "Failed to load sound %s of type mp3!", filePath.c_str());
		}
		break;
	case SoundHandle::Type::TYPE_WAV:
		{
			YM_ASSERT(drwav_init_file(&soundHandle->wav, filePath.c_str(), NULL), "Failed to load sound %s of type wav!", filePath.c_str());
		}
		break;
	default:
		YM_ASSERT(false, "Failed to load sound %s, unrecognized file type!", filePath.c_str());
		break;
	}
}

void ym::AudioSystem::loadEffectFile(SoundHandle* soundHandle, const std::string& filePath, PaSampleFormat format)
{
	soundHandle->asEffect = true;

	// Get file type.
	std::string ext = filePath.substr(filePath.find_last_of(".") + 1);
	if (ext == "mp3") soundHandle->type = SoundHandle::Type::TYPE_MP3;
	if (ext == "wav") soundHandle->type = SoundHandle::Type::TYPE_WAV;

	// Load file.
	switch (soundHandle->type)
	{
	case SoundHandle::Type::TYPE_MP3:
	{
		drmp3_config config;
		if(format == paFloat32)
			soundHandle->directDataF32 = drmp3_open_file_and_read_pcm_frames_f32(filePath.c_str(), &config, &soundHandle->totalFrameCount, NULL);
		else if (format == paInt16)
			soundHandle->directDataI16 = drmp3_open_file_and_read_pcm_frames_s16(filePath.c_str(), &config, &soundHandle->totalFrameCount, NULL);
		soundHandle->nChannels = config.channels;
		soundHandle->sampleRate = config.sampleRate;
	}
	break;
	case SoundHandle::Type::TYPE_WAV:
	{
		drmp3_config config;
		if (format == paFloat32)
			soundHandle->directDataF32 = drwav_open_file_and_read_pcm_frames_f32(filePath.c_str(), &soundHandle->nChannels, &soundHandle->sampleRate, &soundHandle->totalFrameCount, NULL);
		if (format == paInt16)
			soundHandle->directDataI16 = drwav_open_file_and_read_pcm_frames_s16(filePath.c_str(), &soundHandle->nChannels, &soundHandle->sampleRate, &soundHandle->totalFrameCount, NULL);
		YM_ASSERT(drwav_init_file(&soundHandle->wav, filePath.c_str(), NULL), "Failed to load sound %s of type wav!", filePath.c_str());
	}
	break;
	default:
		YM_ASSERT(false, "Failed to load sound %s, unrecognized file type!", filePath.c_str());
		break;
	}
}

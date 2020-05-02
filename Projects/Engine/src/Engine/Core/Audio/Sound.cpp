#include "stdafx.h"
#include "Sound.h"

#include "PortAudio.h"
#include "AudioSystem.h"

ym::Sound::Sound(PortAudio* portAudioPtr) : volume(0.0f), stream(nullptr), portAudioPtr(portAudioPtr), isStreamOn(false)
{
}

ym::Sound::~Sound()
{
	destroy();
}

void ym::Sound::init(PCM::UserData* userData)
{
	this->userData = userData;
	PaStreamParameters outputParameters;
	outputParameters.device = this->portAudioPtr->getDeviceIndex();
	outputParameters.channelCount = 2;       // stereo output
	outputParameters.sampleFormat = this->userData->sampleFormat; // 32 bit floating point output
	outputParameters.suggestedLatency = this->portAudioPtr->getDeviceInfo()->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;

	// Create stream
	PaError err = Pa_OpenStream(
		&this->stream,
		NULL,
		&outputParameters,
		SAMPLE_RATE,
		paFramesPerBufferUnspecified,        // frames per buffer
		paClipOff,
		&PCM::paCallbackNormalPCM,
		this->userData);
	PORT_AUDIO_CHECK(err, "Failed to open default stream!");
}

void ym::Sound::destroy()
{
	if (isCreated())
	{
		stop();
		PORT_AUDIO_CHECK(Pa_CloseStream(this->stream), "Failed to close stream!");
		this->stream = nullptr;
		SAFE_DELETE(this->userData);
	}
}

void ym::Sound::play()
{
	stop();
	PORT_AUDIO_CHECK(Pa_StartStream(this->stream), "Failed to start stream!");
	this->isStreamOn = true;
}

void ym::Sound::pause()
{
	
}

void ym::Sound::unpause()
{
	
}

void ym::Sound::stop()
{
	if (this->isStreamOn)
	{
		PCM::resetPos(&this->userData->handle);
		PORT_AUDIO_CHECK(Pa_StopStream(this->stream), "Failed to stop stream!");
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
	if (this->userData != nullptr)
	{
		userData->volume = volume;
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
	return this->stream != nullptr;
}

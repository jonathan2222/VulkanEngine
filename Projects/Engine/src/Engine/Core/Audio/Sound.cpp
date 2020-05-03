#include "stdafx.h"
#include "Sound.h"

#include "PortAudio.h"
#include "AudioSystem.h"

ym::Sound::Sound(PortAudio* portAudioPtr) : volume(0.0f), userData(nullptr), stream(nullptr), portAudioPtr(portAudioPtr), isStreamOn(false)
{
}

ym::Sound::~Sound()
{
	destroy();
}

void ym::Sound::init(PCM::UserData* userData, PCM::Func pcmFunction)
{
	this->userData = userData;
	PaStreamParameters outputParameters;
	outputParameters.device = this->portAudioPtr->getDeviceIndex();
	outputParameters.channelCount = 2;
	outputParameters.sampleFormat = this->userData->sampleFormat;
	outputParameters.suggestedLatency = this->portAudioPtr->getDeviceInfo()->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;

	// Create stream
	PaError err = Pa_OpenStream(
		&this->stream,
		NULL,
		&outputParameters,
		userData->handle.sampleRate,
		paFramesPerBufferUnspecified,        // frames per buffer
		paClipOff,
		PCM::getCallbackFunction(pcmFunction),
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

		// Delete sound data.
		if (this->userData->handle.asEffect)
		{
			if (this->userData->handle.directDataF32 != nullptr)
			{
				if (this->userData->handle.type == SoundHandle::TYPE_MP3)
					drmp3_free(this->userData->handle.directDataF32, nullptr);
				if (this->userData->handle.type == SoundHandle::TYPE_WAV)
					drwav_free(this->userData->handle.directDataF32, nullptr);
			}
			if (this->userData->handle.directDataI16 != nullptr)
			{
				if (this->userData->handle.type == SoundHandle::TYPE_MP3)
					drmp3_free(this->userData->handle.directDataI16, nullptr);
				if (this->userData->handle.type == SoundHandle::TYPE_WAV)
					drwav_free(this->userData->handle.directDataI16, nullptr);
			}
		}
		else
		{
			if (this->userData->handle.type == SoundHandle::TYPE_MP3)
				drmp3_uninit(&this->userData->handle.mp3);
			if (this->userData->handle.type == SoundHandle::TYPE_WAV)
				drwav_uninit(&this->userData->handle.wav);
		}
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
	if (this->isStreamOn)
	{
		PORT_AUDIO_CHECK(Pa_StopStream(this->stream), "Failed to stop stream!");
		PCM::setPos(&this->userData->handle, this->userData->handle.pos);
		this->userData->finished = false;
		this->isStreamOn = false;
	}
}

void ym::Sound::unpause()
{
	play();
}

void ym::Sound::stop()
{
	if (this->isStreamOn)
	{
		PORT_AUDIO_CHECK(Pa_StopStream(this->stream), "Failed to stop stream!");
		PCM::setPos(&this->userData->handle, 0);
		this->userData->finished = false;
		this->isStreamOn = false;
	}
}

float ym::Sound::getVolume() const
{
	return this->volume;
}

void ym::Sound::applyVolume(float volumeChange)
{
	this->volume += volumeChange;
	setVolume(std::clamp(this->volume, 0.f, 2.f));
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
	this->userData->loop = state;
}

void ym::Sound::setDistance(float distance)
{
	this->userData->distance = distance;
}

bool ym::Sound::isCreated() const
{
	return this->stream != nullptr;
}

#include "stdafx.h"
#include "Sound.h"

#include "PortAudio.h"
#include "AudioSystem.h"

#include "Engine/Core/Camera.h"

ym::Sound::Sound(PortAudio* portAudioPtr) : volume(0.0f), userData(nullptr), stream(nullptr), portAudioPtr(portAudioPtr), isStreamOn(false)
{
}

ym::Sound::~Sound()
{
	destroy();
}

void ym::Sound::init(PCM::UserData* userData)
{
	this->userData = userData;
	//this->userData->delayBuffer.init(userData->handle.sampleRate);

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
		paFramesPerBufferUnspecified,
		paClipOff,
		PCM::paCallbackPCM,
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

		// Clear filters.
		for (Filter* filter : this->userData->filters)
		{
			filter->destroy();
			delete filter;
		}
		this->userData->filters.clear();

		//this->userData->delayBuffer.destroy();
		SAFE_DELETE(this->userData);
	}
}

void ym::Sound::play()
{
	stop();
	//this->userData->delayBuffer.clear();
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

void ym::Sound::setName(const std::string& name)
{
	this->name = name;
}

std::string ym::Sound::getName() const
{
	return this->name;
}

glm::vec3 ym::Sound::getPosition() const
{
	return this->userData->soundData.sourcePos;
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
		userData->soundData.volume = volume;
	}
}

void ym::Sound::setGroupVolume(float volume)
{
	this->userData->soundData.groupVolume = volume;
}

void ym::Sound::setMasterVolume(float volume)
{
	this->userData->soundData.masterVolume = volume;
}

void ym::Sound::setLoop(bool state)
{
	this->userData->soundData.loop = state;
}

void ym::Sound::addFilter(Filter* filter)
{
	filter->init(userData->handle.sampleRate);
	this->userData->filters.push_back(filter);
}

std::vector<ym::Filter*>& ym::Sound::getFilters()
{
	return this->userData->filters;
}

void ym::Sound::setSourcePosition(const glm::vec3& sourcePos)
{
	this->userData->soundData.sourcePos = sourcePos;
}

void ym::Sound::setReceiverPosition(const glm::vec3& receiverPos)
{
	this->userData->soundData.receiverPos = receiverPos;
}

void ym::Sound::setReceiverDir(const glm::vec3& receiverDir)
{
	YM_LOG_WARN("setReceiverDir is not implemented!");
	//this->userData->receiverDir = receiverDir;
}

void ym::Sound::setReceiverLeft(const glm::vec3& receiverLeft)
{
	this->userData->soundData.receiverLeft = receiverLeft;
}

void ym::Sound::setReceiverUp(const glm::vec3& receiverUp)
{
	this->userData->soundData.receiverUp = receiverUp;
}

void ym::Sound::setReceiver(Camera* camera)
{
	setReceiverPosition(camera->getPosition());
	setReceiverLeft(-camera->getRight());
	setReceiverUp(camera->getUp());
}

/*
void ym::Sound::setCutoffFrequency(float fc)
{
	this->userData->fc = fc;
}

float ym::Sound::getCutoffFrequency() const
{
	return this->userData->fc;
}

uint32_t ym::Sound::getSampleRate() const
{
	return this->userData->handle.sampleRate;
}

void ym::Sound::setEchoData(float delay, float gain)
{
	this->userData->echoDelay = delay;
	this->userData->echoGain = gain;
}
*/

bool ym::Sound::isCreated() const
{
	return this->stream != nullptr;
}

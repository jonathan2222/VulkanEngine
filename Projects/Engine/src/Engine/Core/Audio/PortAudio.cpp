#include "stdafx.h"
#include "PortAudio.h"

ym::PortAudio::PortAudio() : deviceIndex(0), deviceInfo(nullptr), errorCode(paNoError)
{
}

ym::PortAudio::~PortAudio()
{
}

void ym::PortAudio::init()
{
	this->errorCode = Pa_Initialize();
	PORT_AUDIO_CHECK(this->errorCode, "Failed to Initialize PortAudio!");

	this->deviceIndex = Pa_GetDefaultOutputDevice();
	this->deviceInfo = Pa_GetDeviceInfo(this->deviceIndex);
}

void ym::PortAudio::destroy()
{
	if (this->errorCode == paNoError)
	{
		Pa_Terminate();
		YM_LOG_INFO("Destroyed Port Audio.");
	}
}

PaDeviceIndex ym::PortAudio::getDeviceIndex()
{
	return this->deviceIndex;
}

const PaDeviceInfo* ym::PortAudio::getDeviceInfo()
{
	return this->deviceInfo;
}

#include "stdafx.h"
#include "EchoFilter.h"

void ym::EchoFilter::init(uint64_t sampleRate)
{
	this->delayBuffer.init(sampleRate);
}

void ym::EchoFilter::destroy()
{
}

void ym::EchoFilter::begin()
{
	this->delayBuffer.setDelay(this->delay);
}

std::pair<float, float> ym::EchoFilter::process(SoundData* data, float left, float right)
{
	float oldLeftSampel = this->delayBuffer.get();
	float newLeftInput = left + this->gain * oldLeftSampel;
	this->delayBuffer.add(newLeftInput);

	float oldRightSampel = this->delayBuffer.get();
	float newRightInput = right + this->gain * oldRightSampel;
	this->delayBuffer.add(newRightInput);

	return std::pair<float, float>(oldLeftSampel, oldRightSampel);
}

std::string ym::EchoFilter::getName() const
{
	return "Echo Filter";
}

void ym::EchoFilter::setDelay(float delay)
{
	if (glm::abs(this->delay - delay) > 0.0001f)
		this->delayBuffer.clear();
	this->delay = delay;
}

void ym::EchoFilter::setGain(float gain)
{
	this->gain = gain;
}

float ym::EchoFilter::getDelay() const
{
	return this->delay;
}

float ym::EchoFilter::getGain() const
{
	return this->gain;
}

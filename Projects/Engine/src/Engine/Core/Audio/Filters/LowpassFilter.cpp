#include "stdafx.h"
#include "LowpassFilter.h"

#include <glm/gtc/constants.hpp>

void ym::LowpassFilter::init(uint64_t sampleRate)
{
	this->sampleRate = (float)sampleRate;
	this->delayBuffer.init(sampleRate);
}

void ym::LowpassFilter::destroy()
{
}

void ym::LowpassFilter::begin()
{
	this->delayBuffer.setDelayInSamples(1); // One sample per ear
}

std::pair<float, float> ym::LowpassFilter::process(SoundData* data, float left, float right)
{
	float oldLeftSampel = this->delayBuffer.get();	// get y(n-1)
	float oldRightSampel = this->delayBuffer.get(); // get y(n-1)

	// Lowpass calculations
	constexpr float PI = glm::pi<float>();
	const float fs = this->sampleRate;
	float product = 2.f * PI * this->cutoffFrequency / fs;
	float tmp = (2.f - glm::cos(product));
	float b = glm::sqrt(tmp * tmp - 1.f) - 2.f + glm::cos(product);
	float a = 1.f + b;

	// Apply y(n) = b*x(n) - a*y(n-1)
	float newLeft = a * left - b * oldLeftSampel;
	float newRight = a * right - b * oldRightSampel;

	this->delayBuffer.add(left);
	this->delayBuffer.add(right);
	return std::pair<float, float>(newLeft, newRight);
}

std::string ym::LowpassFilter::getName() const
{
	return "Lowpass Filter";
}

void ym::LowpassFilter::setCutoffFrequency(float cutoffFrequency)
{
	if (glm::abs(this->cutoffFrequency - cutoffFrequency) > 0.0001f)
		this->delayBuffer.clear();
	this->cutoffFrequency = cutoffFrequency;
}

float ym::LowpassFilter::getCutoffFrequency() const
{
	return this->cutoffFrequency;
}

float ym::LowpassFilter::getSampleRate() const
{
	return this->sampleRate;
}

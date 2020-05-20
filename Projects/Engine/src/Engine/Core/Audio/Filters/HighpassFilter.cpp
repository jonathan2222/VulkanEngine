#include "stdafx.h"
#include "HighpassFilter.h"

#include <glm/gtc/constants.hpp>
#include "Utils/Utils.h"

void ym::HighpassFilter::init(uint64_t sampleRate)
{
	this->sampleRate = (float)sampleRate;
	this->cutoffFrequency = this->sampleRate * 0.25f;
	this->delayBuffer.init(sampleRate);
}

void ym::HighpassFilter::destroy()
{
}

void ym::HighpassFilter::begin()
{
	this->delayBuffer.setDelayInSamples(1); // One sample per ear
}

std::pair<float, float> ym::HighpassFilter::process(SoundData* data, float left, float right)
{
	// Highpass calculations
	constexpr float PI = glm::pi<float>();
	const float fs = this->sampleRate;
	float product = 2.f * PI * this->cutoffFrequency / fs;
	float tmp = (2.f - glm::cos(product));
	float b = 2.f - glm::cos(product) - glm::sqrt(tmp * tmp - 1.f);
	float a = 1.f - b;

	// Apply y(n) = b*x(n) - a*y(n-1)
	float oldLeftSampel = this->delayBuffer.get();	// get y(n-1)
	float newLeft = a * left - b * oldLeftSampel;
	this->delayBuffer.add(newLeft);

	float oldRightSampel = this->delayBuffer.get(); // get y(n-1)
	float newRight = a * right - b * oldRightSampel;
	this->delayBuffer.add(newRight);

	return std::pair<float, float>(newLeft, newRight);
}

std::string ym::HighpassFilter::getName() const
{
	return "Highpass Filter";
}

void ym::HighpassFilter::setCutoffFrequency(float cutoffFrequency)
{
	if (glm::abs(this->cutoffFrequency - cutoffFrequency) > 0.0001f)
		this->delayBuffer.clear();
	this->cutoffFrequency = cutoffFrequency;
}

float ym::HighpassFilter::getCutoffFrequency() const
{
	return this->cutoffFrequency;
}

float ym::HighpassFilter::getSampleRate() const
{
	return this->sampleRate;
}

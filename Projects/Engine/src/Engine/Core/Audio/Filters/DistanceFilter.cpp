#include "stdafx.h"
#include "DistanceFilter.h"

#include "../SoundData.h"
#include <glm/gtx/vector_angle.hpp>

void ym::DistanceFilter::init(uint64_t sampleRate)
{
}

void ym::DistanceFilter::destroy()
{
}

void ym::DistanceFilter::begin()
{
}

std::pair<float, float> ym::DistanceFilter::process(SoundData* data, float left, float right)
{
	auto map = [](float x, float min, float max, float nMin, float nMax) {
		return nMin + (x - min) * (nMax - nMin) / (max - min);
	};

	glm::vec3 receiverToSource = data->sourcePos - data->receiverPos;
	float distance = glm::length(receiverToSource);
	float attenuation = distance <= 0.000001f ? 0.f : 1.f / (distance * distance);

	// Angle from left to right [0, pi]
	receiverToSource = glm::normalize(receiverToSource);
	float angle = glm::orientedAngle(receiverToSource, data->receiverLeft, data->receiverUp);

	// Map angle to [0, pi/2]
	constexpr float pi = glm::pi<float>();
	if (distance <= 0.000001f) angle = pi * .5f;
	angle = map(angle, 0.f, pi, 0.f, pi * 0.5f);

	// Stereo panning
	float c = glm::cos(angle);
	float s = glm::sin(angle);
	float leftGain = c * c;
	float rightGain = s * s;

	float leftEar = left * attenuation * leftGain;
	float rightEar = right * attenuation * rightGain;
	return std::pair<float, float>(leftEar, rightEar);
}

std::string ym::DistanceFilter::getName() const
{
	return "Distance Filter";
}

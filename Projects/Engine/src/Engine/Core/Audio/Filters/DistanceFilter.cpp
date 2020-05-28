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

	auto mix = [](float a, float b, float t) -> float {
		return a * (1.f - t) + b * t;
	};

	glm::vec3 receiverToSource = data->sourcePos - data->receiverPos;
	float distance = glm::length(receiverToSource);
	float attenuation = distance * distance;
	attenuation = glm::min(attenuation <= 0.000001f ? 0.f : 1.f / attenuation, 1.f);

	// Calculate angle from left to right [0, pi]
	receiverToSource = glm::normalize(receiverToSource);
	float cosAngle = glm::clamp(glm::dot(receiverToSource, data->receiverLeft), -1.f, 1.f);
	float angle = glm::acos(cosAngle);

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

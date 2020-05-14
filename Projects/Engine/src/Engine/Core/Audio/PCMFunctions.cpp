#include "stdafx.h"
#include "PCMFunctions.h"

#include <limits>
#include <glm/gtx/vector_angle.hpp>

//PaStreamCallback* ym::PCM::getCallbackFunction(Func function)
//{
//	switch (function)
//	{
//	case ym::PCM::Func::DISTANCE:
//		return paCallbackDistancePCM;
//		break;
//	case ym::PCM::Func::ECHO:
//		return paCallbackEchoPCM;
//		break;
//	case ym::PCM::Func::ECHO_DISTANCE:
//		return paCallbackEchoDistancePCM;
//		break;
//	case ym::PCM::Func::LOWPASS:
//		return paCallbackLowpassPCM;
//		break;
//	case ym::PCM::Func::NORMAL:
//	default:
//		return paCallbackNormalPCM;
//		break;
//	}
//}

void ym::PCM::setPos(SoundHandle* handle, uint64_t newPos)
{
	handle->pos = newPos;
	if (handle->asEffect == false)
	{
		switch (handle->type)
		{
		case SoundHandle::Type::TYPE_MP3:
			drmp3_seek_to_pcm_frame(&handle->mp3, newPos);
			break;
		case SoundHandle::Type::TYPE_WAV:
			drwav_seek_to_pcm_frame(&handle->wav, newPos);
			break;
		default:
			break;
		}
	}
}

int ym::PCM::paCallbackPCM(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{
	UserData* data = (UserData*)userData;
	uint64_t framesRead = readPCM(data, framesPerBuffer, outputBuffer);
	OutputData outputData = getOutputData(data, outputBuffer);

	for (Filter* ft : data->filters)
		ft->begin();
	bool shouldContinue = false;
	for (uint64_t t = 0; t < framesPerBuffer; t++)
	{
		auto samples = getSamples(data, &outputData);
		float leftEar = samples.first;
		float rightEar = samples.second;
		for (Filter* ft : data->filters)
		{
			auto res = ft->process(&data->soundData, leftEar, rightEar);
			leftEar = std::get<0>(res);
			rightEar = std::get<1>(res);
			shouldContinue = shouldContinue && (abs(leftEar) > 0.00000001f ? true : shouldContinue);
			shouldContinue = shouldContinue && (abs(rightEar) > 0.00000001f ? true : shouldContinue);

		}
		applyToEar(data, &outputData, { leftEar * data->soundData.volume, rightEar * data->soundData.volume });
	}

	return finish(data, framesRead, framesPerBuffer, shouldContinue);
}

int ym::PCM::paCallbackNormalPCM(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{
	UserData* data = (UserData*)userData;
	uint64_t framesRead = readPCM(data, framesPerBuffer, outputBuffer);
	OutputData outputData = getOutputData(data, outputBuffer);

	for (uint64_t t = 0; t < framesRead; t++)
	{
		auto samples = getSamples(data, &outputData);
		float leftEar = (float)(samples.first * data->soundData.volume); // Left
		float rightEar = (float)(samples.second * data->soundData.volume); // Right
		applyToEar(data, &outputData, { leftEar, rightEar });
	}

	// Set the continue state to false because no delay effect was used => can quit because all samples have been loaded.
	return finish(data, framesRead, framesPerBuffer, false);
}
/*
int ym::PCM::paCallbackDistancePCM(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{
	UserData* data = (UserData*)userData;
	uint64_t framesRead = readPCM(data, framesPerBuffer, outputBuffer);
	OutputData outputData = getOutputData(data, outputBuffer);
	
	auto map = [](float x, float min, float max, float nMin, float nMax) {
		return nMin + (x - min) * (nMax - nMin) / (max - min);
	};

	for (uint64_t t = 0; t < framesRead; t++)
	{
		glm::vec3 receiverToSource = data->sourcePos - data->receiverPos;
		float distance = glm::length(receiverToSource);
		float attenuation = distance <= 0.000001f ? 0.f : 1.f / (distance * distance);

		// Angle from left to right [0, pi]
		receiverToSource = glm::normalize(receiverToSource);
		float angle = glm::orientedAngle(receiverToSource, data->receiverLeft, data->receiverUp);

		// Map angle to [0, pi/2]
		constexpr float pi = glm::pi<float>();
		if (distance <= 0.000001f) angle = pi*.5f;
		angle = map(angle, 0.f, pi, 0.f, pi*0.5f);

		// Stereo panning
		float c = glm::cos(angle);
		float s = glm::sin(angle);
		float leftGain = c * c;
		float rightGain = s * s;

		auto samples = getSamples(data, &outputData);
		float leftEar = (float)(samples.first * data->volume * attenuation * leftGain);
		float rightEar = (float)(samples.second * data->volume * attenuation * rightGain);
		applyToEar(data, &outputData, { leftEar, rightEar });
	}

	// Set the continue state to false because no delay effect was used => can quit because all samples have been loaded.
	return finish(data, framesRead, framesPerBuffer, false);
}

int ym::PCM::paCallbackLowpassPCM(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{
	UserData* data = (UserData*)userData;
	uint64_t framesRead = readPCM(data, framesPerBuffer, outputBuffer);
	OutputData outputData = getOutputData(data, outputBuffer);

	data->delayBuffer.setDelayInSamples(1); // One sample per ear
	bool shouldContinue = false;
	for (uint64_t t = 0; t < framesPerBuffer; t++)
	{
		float oldLeftSampel = data->delayBuffer.get();	// get y(n-1)
		float oldRightSampel = data->delayBuffer.get(); // get y(n-1)
		shouldContinue = abs(oldLeftSampel) > 0.00000001f ? true : shouldContinue;
		shouldContinue = abs(oldRightSampel) > 0.00000001f ? true : shouldContinue;
		auto samples = getSamples(data, &outputData); // get x(n)

		// Lowpass calculations
		constexpr float PI = glm::pi<float>();
		const float fs = data->handle.sampleRate;
		float product = 2.f * PI * data->fc / fs;
		float tmp = (2.f - glm::cos(product));
		float b = glm::sqrt(tmp*tmp - 1.f) - 2.f + glm::cos(product);
		float a = 1.f + b;

		// Apply y(n) = b*x(n) - a*y(n-1)
		float left = (a * samples.first - b * oldLeftSampel) * data->volume;
		float right = (a * samples.second - b * oldRightSampel) * data->volume;

		applyToEar(data, &outputData, { left, right });

		data->delayBuffer.add(left);
		data->delayBuffer.add(right);
	}

	return finish(data, framesRead, framesPerBuffer, shouldContinue);
}

int ym::PCM::paCallbackEchoDistancePCM(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{
	UserData* data = (UserData*)userData;
	uint64_t framesRead = readPCM(data, framesPerBuffer, outputBuffer);
	OutputData outputData = getOutputData(data, outputBuffer);

	auto map = [](float x, float min, float max, float nMin, float nMax) {
		return nMin + (x - min) * (nMax - nMin) / (max - min);
	};

	data->delayBuffer.setDelay(data->echoDelay);
	bool shouldContinue = false;
	for (uint64_t t = 0; t < framesPerBuffer; t++)
	{
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

		float oldLeftSampel = data->delayBuffer.get() * attenuation * leftGain;
		float oldRightSampel = data->delayBuffer.get() * attenuation * rightGain;
		shouldContinue = abs(oldLeftSampel) > 0.00000001f ? true : shouldContinue;
		shouldContinue = abs(oldRightSampel) > 0.00000001f ? true : shouldContinue;
		auto samples = getSamples(data, &outputData);
		applyToEar(data, &outputData, { oldLeftSampel, oldRightSampel });

		// Left
		float leftEar = (float)(samples.first * data->volume);
		float newLeftInput = leftEar + data->echoGain * oldLeftSampel;

		// Right
		float rightEar = (float)(samples.second * data->volume);
		float newRightInput = rightEar + data->echoGain * oldRightSampel;

		data->delayBuffer.add(newLeftInput);
		data->delayBuffer.add(newRightInput);
	}

	return finish(data, framesRead, framesPerBuffer, shouldContinue);
}

int ym::PCM::paCallbackEchoPCM(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{
	UserData* data = (UserData*)userData;
	uint64_t framesRead = readPCM(data, framesPerBuffer, outputBuffer);
	OutputData outputData = getOutputData(data, outputBuffer);

	data->delayBuffer.setDelay(data->echoDelay);
	bool shouldContinue = false;
	for (uint64_t t = 0; t < framesPerBuffer; t++)
	{
		float oldLeftSampel = data->delayBuffer.get();
		float oldRightSampel = data->delayBuffer.get();
		shouldContinue = abs(oldLeftSampel) > 0.00000001f ? true : shouldContinue;
		shouldContinue = abs(oldRightSampel) > 0.00000001f ? true : shouldContinue;
		auto samples = getSamples(data, &outputData);
		applyToEar(data, &outputData, { oldLeftSampel, oldRightSampel });

		// Left
		float leftEar = (float)(samples.first * data->volume);
		float newLeftInput = leftEar + data->echoGain * oldLeftSampel;

		// Right
		float rightEar = (float)(samples.second * data->volume);
		float newRightInput = rightEar + data->echoGain * oldRightSampel;

		data->delayBuffer.add(newLeftInput);
		data->delayBuffer.add(newRightInput);
	}

	return finish(data, framesRead, framesPerBuffer, shouldContinue);
}

ym::PCM::OutputData* ym::PCM::echoFilter(UserData* data, OutputData* outputData, uint64_t framesRead)
{
	data->delayBuffer.setDelay(data->echoDelay);
	for (uint64_t t = 0; t < framesRead; t++)
	{
		// Left
		float sample = getSample(data, outputData);
		float oldSample = data->delayBuffer.get();
		applyToEar(data, outputData, oldSample);
		data->delayBuffer.add(sample + data->echoGain * oldSample);

		// Right
		sample = getSample(data, outputData);
		oldSample = data->delayBuffer.get();
		applyToEar(data, outputData, oldSample);
		data->delayBuffer.add(sample + data->echoGain * oldSample);
	}
	return outputData;
}*/

uint64_t ym::PCM::readPCMFrames(SoundHandle* handle, uint64_t framesToRead, PaSampleFormat format, void* outBuffer)
{
	drmp3_uint64 frameCount = (drmp3_uint64)framesToRead;
	uint64_t framesRead = 0;
	switch (handle->type)
	{
	case SoundHandle::Type::TYPE_MP3:
		if (format == paFloat32) framesRead = drmp3_read_pcm_frames_f32(&handle->mp3, frameCount, (float*)outBuffer);
		if (format == paInt16) framesRead = drmp3_read_pcm_frames_s16(&handle->mp3, frameCount, (int16_t*)outBuffer);
		break;
	case SoundHandle::Type::TYPE_WAV:
		if (format == paFloat32) framesRead = drwav_read_pcm_frames_f32(&handle->wav, frameCount, (float*)outBuffer);
		if (format == paInt16) framesRead = drwav_read_pcm_frames_s16(&handle->wav, frameCount, (int16_t*)outBuffer);
		break;
	default:
		break;
	}
	handle->pos += frameCount;
	return framesRead;
}

uint64_t ym::PCM::readPCMFramesEffect(SoundHandle* handle, uint64_t framesToRead, PaSampleFormat format, void* outBuffer)
{
	uint64_t framesRead = framesToRead;
	// Do not copy data if sound is already processed. 
	// - This is bad, I could have looped around but then the readPCMFrames function would not work (Because the dr library does not do this).
	if (handle->pos >= handle->totalFrameCount)
		framesRead = 0;
	else
	{
		// Copy frames to the outBuffer.
		framesRead = std::min(framesToRead, handle->totalFrameCount - handle->pos);
		if (format == paFloat32)
			memcpy(outBuffer, &handle->directDataF32[handle->pos * 2], framesRead * 2 * sizeof(float));
		else if (format == paInt16)
			memcpy(outBuffer, &handle->directDataI16[handle->pos * 2], framesRead * 2 * sizeof(int16_t));
		handle->pos += framesRead;
	}
	return framesRead;
}

float ym::PCM::getSample(UserData* data, OutputData* outputData)
{
	return data->sampleFormat == paInt16 ? (float)(*(outputData->outI16)) : *(outputData->outF32);
}

std::pair<float, float> ym::PCM::getSamples(UserData* data, OutputData* outputData)
{
	float left = data->sampleFormat == paInt16 ? (float)(*(outputData->outI16)) : *(outputData->outF32);
	float right = data->sampleFormat == paInt16 ? (float)(*(outputData->outI16+1)) : *(outputData->outF32+1);
	return std::pair<float, float>(left, right);
}

ym::PCM::OutputData ym::PCM::getOutputData(UserData* data, void* out)
{
	OutputData outData;
	outData.outF32 = (float*)out;
	outData.outI16 = (int16_t*)out;
	return outData;
}

void ym::PCM::applyToEar(UserData* data, OutputData* outputData, float value)
{
	if (data->sampleFormat == paInt16)
		*outputData->outI16++ = (int16_t)std::clamp(value, (float)INT16_MIN, (float)INT16_MAX);
	else if (data->sampleFormat == paFloat32)
		*outputData->outF32++ = value;
}

void ym::PCM::applyToEar(UserData* data, OutputData* outputData, std::pair<float, float> values)
{
	applyToEar(data, outputData, values.first);
	applyToEar(data, outputData, values.second);
}

uint64_t ym::PCM::readPCM(UserData* data, uint64_t framesPerBuffer, void* outBuffer)
{
	uint64_t framesRead = 0;
	if (data->handle.asEffect)
		framesRead = readPCMFramesEffect(&data->handle, framesPerBuffer, data->sampleFormat, outBuffer);
	else
		framesRead = readPCMFrames(&data->handle, framesPerBuffer, data->sampleFormat, outBuffer);
	return framesRead;
}

int ym::PCM::finish(UserData* data, uint64_t framesRead, uint64_t framesPerBuffer, bool continueFlag)
{
	if (data->soundData.loop)
	{
		// Reset and play again.
		if (framesRead < framesPerBuffer && !continueFlag)
			setPos(&data->handle, 0);
		return paContinue;
	}
	else
	{
		// Reset and stop the stream.
		if (framesRead < framesPerBuffer && !continueFlag)
		{
			setPos(&data->handle, 0);
			data->finished = true;
		}
		return data->finished ? paComplete : paContinue;
	}
}

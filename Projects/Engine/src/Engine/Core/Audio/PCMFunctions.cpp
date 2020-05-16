#include "stdafx.h"
#include "PCMFunctions.h"

#include <limits>
#include <glm/gtx/vector_angle.hpp>

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

		float volume = data->soundData.volume * data->soundData.groupVolume * data->soundData.masterVolume;
		applyToEar(data, &outputData, { leftEar * volume, rightEar * volume });
	}

	return finish(data, framesRead, framesPerBuffer, shouldContinue);
}

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

#include "stdafx.h"
#include "PCMFunctions.h"

uint64_t ym::PCM::readPCMFrames(SoundHandle* handle, uint64_t framesToRead, PaSampleFormat format, void* outBuffer)
{
	drmp3_uint64 frameCount = (drmp3_uint64)framesToRead;
	switch (handle->type)
	{
	case SoundHandle::Type::TYPE_MP3:
		if (format == paFloat32) return drmp3_read_pcm_frames_f32(&handle->mp3, frameCount, (float*)outBuffer);
		if (format == paInt16) return drmp3_read_pcm_frames_s16(&handle->mp3, frameCount, (int16_t*)outBuffer);
		break;
	case SoundHandle::Type::TYPE_WAV:
		if (format == paFloat32) return drwav_read_pcm_frames_f32(&handle->wav, frameCount, (float*)outBuffer);
		if (format == paInt16) return drwav_read_pcm_frames_s16(&handle->wav, frameCount, (int16_t*)outBuffer);
		break;
	default:
		return 0;
		break;
	}
}

void ym::PCM::resetPos(SoundHandle* handle)
{
	switch (handle->type)
	{
	case SoundHandle::Type::TYPE_MP3:
		drmp3_seek_to_pcm_frame(&handle->mp3, 0);
		break;
	case SoundHandle::Type::TYPE_WAV:
		drwav_seek_to_pcm_frame(&handle->wav, 0);
		break;
	default:
		break;
	}
}

int ym::PCM::paCallbackNormalPCM(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{
	UserData* data = (UserData*)userData;
	void* out = nullptr;
	if (data->sampleFormat == paInt16)
		out = (int16_t*)outputBuffer;
	else if (data->sampleFormat == paFloat32)
		out = (float*)outputBuffer;

	uint64_t framesRead = readPCMFrames(&data->handle, framesPerBuffer, data->sampleFormat, out);
	
	float* outF32 = (float*)out;
	int16_t * outI16 = (int16_t*)out;
	for (uint64_t t = 0; t < framesPerBuffer; t++)
	{
		if (data->sampleFormat == paInt16)
		{
			int16_t sample = *outI16;
			*outI16++ = (int16_t)std::clamp((float)sample * std::clamp(data->volume, 0.f, 1.f), (float)INT16_MIN, (float)INT16_MAX); // Left
			sample = *outI16;
			*outI16++ = (int16_t)std::clamp((float)sample * std::clamp(data->volume, 0.f, 1.f), (float)INT16_MIN, (float)INT16_MAX); // Right
		}
		else if (data->sampleFormat == paFloat32)
		{
			float sample = *outF32;
			*outF32++ = (float)(sample * data->volume); // Left
			sample = *outF32;
			*outF32++ = (float)(sample * data->volume); // Right
		}
	}

	if (framesRead == 0) data->finished = true;

	return framesRead > 0 ? paContinue : paComplete;
}

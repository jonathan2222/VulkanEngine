#include "stdafx.h"
#include "PCMFunctions.h"

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

int ym::PCM::paCallbackNormalPCM(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{
	UserData* data = (UserData*)userData;
	void* out = (float*)outputBuffer;
	if (data->sampleFormat == paInt16)
		out = (int16_t*)outputBuffer;

	uint64_t framesRead = 0;
	if (data->handle.asEffect)
		framesRead = readPCMFramesEffect(&data->handle, framesPerBuffer, data->sampleFormat, out);
	else
		framesRead = readPCMFrames(&data->handle, framesPerBuffer, data->sampleFormat, out);
	
	float* outF32 = (float*)out;
	int16_t * outI16 = (int16_t*)out;
	for (uint64_t t = 0; t < framesRead; t++)
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

	if (data->loop)
	{
		// Reset and play again.
		if (framesRead < framesPerBuffer)
			setPos(&data->handle, 0);
		return paContinue;
	}
	else
	{
		// Reset and stop the stream.
		if (framesRead < framesPerBuffer)
		{
			setPos(&data->handle, 0);
			data->finished = true;
		}
		return data->finished ? paComplete : paContinue;
	}
}

#pragma once

#include "PortAudio.h"
#include "DR/dr_helper.h"
#include <cstdint>
#include <algorithm>

namespace ym
{
	class PCM
	{
	public:
		enum class Func
		{
			NORMAL,
			DISTANCE
		};

		struct UserData
		{
			ym::SoundHandle handle;
			bool finished{ false };
			PaSampleFormat sampleFormat{ paFloat32 };

			float volume{ 1.f };
			bool loop{ false };

			float distance{ 0.f };
		};

		static PaStreamCallback* getCallbackFunction(Func function);

		static uint64_t readPCMFrames(SoundHandle* handle, uint64_t framesToRead, PaSampleFormat format, void* outBuffer);
		static void setPos(SoundHandle* handle, uint64_t newPos);
		static uint64_t readPCMFramesEffect(SoundHandle* handle, uint64_t framesToRead, PaSampleFormat format, void* outBuffer);

		static int paCallbackNormalPCM(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
			const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData);

		static int paCallbackDistancePCM(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
			const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData);
	};
}
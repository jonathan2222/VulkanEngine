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
		struct UserData
		{
			ym::SoundHandle handle;
			bool finished{ false };
			PaSampleFormat sampleFormat{ paFloat32 };

			float volume{ 1.f };
		};

		static uint64_t readPCMFrames(SoundHandle* handle, uint64_t framesToRead, PaSampleFormat format, void* outBuffer);
		static void resetPos(SoundHandle* handle);

		static int paCallbackNormalPCM(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
			const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData);
	};
}
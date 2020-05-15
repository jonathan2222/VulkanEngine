#pragma once

#include "PortAudio.h"
#include "DR/dr_helper.h"
#include "Filters/Filter.h"
#include "SoundData.h"

#include <cstdint>
#include <algorithm>
#include <vector>
#include <glm/glm.hpp>

namespace ym
{
	class PCM
	{
	private:
		struct OutputData
		{
			float* outF32 = nullptr;
			int16_t* outI16 = nullptr;
		};

	public:

		struct UserData
		{
			SoundData soundData;

			ym::SoundHandle handle;
			bool finished{ false };
			PaSampleFormat sampleFormat{ paFloat32 };
			std::vector<Filter*> filters;
		};

		static void setPos(SoundHandle* handle, uint64_t newPos);

		static int paCallbackPCM(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
			const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData);

	private:
		static uint64_t readPCMFrames(SoundHandle* handle, uint64_t framesToRead, PaSampleFormat format, void* outBuffer);
		static uint64_t readPCMFramesEffect(SoundHandle* handle, uint64_t framesToRead, PaSampleFormat format, void* outBuffer);
		static std::pair<float, float> getSamples(UserData* data, OutputData* outputData);
		static OutputData getOutputData(UserData* data, void* out);
		static void applyToEar(UserData* data, OutputData* outputData, float value);
		static void applyToEar(UserData* data, OutputData* outputData, std::pair<float, float> values);
		static uint64_t readPCM(UserData* data, uint64_t framesPerBuffer, void* outBuffer);
		static int finish(UserData* data, uint64_t framesRead, uint64_t framesPerBuffer, bool continueFlag);

	};
}
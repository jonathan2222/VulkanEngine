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

			glm::vec3 sourcePos;
			glm::vec3 receiverPos;
			glm::vec3 receiverDir;
			glm::vec3 receiverLeft;
			glm::vec3 receiverUp;
		};

		static PaStreamCallback* getCallbackFunction(Func function);

		static void setPos(SoundHandle* handle, uint64_t newPos);

		static int paCallbackNormalPCM(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
			const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData);
		static int paCallbackDistancePCM(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
			const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData);

	private:
		struct OutputData
		{
			float* outF32 = nullptr;
			int16_t* outI16 = nullptr;
		};

		static uint64_t readPCMFrames(SoundHandle* handle, uint64_t framesToRead, PaSampleFormat format, void* outBuffer);
		static uint64_t readPCMFramesEffect(SoundHandle* handle, uint64_t framesToRead, PaSampleFormat format, void* outBuffer);
		static float getSample(UserData * data, OutputData * outputData);
		static OutputData getOutputData(UserData* data, void* out);
		static void applyToEar(UserData* data, OutputData* outputData, float value);
		static uint64_t readPCM(UserData* data, uint64_t framesPerBuffer, void* outBuffer);
		static int finish(UserData* data, uint64_t framesRead, uint64_t framesPerBuffer);

	};
}
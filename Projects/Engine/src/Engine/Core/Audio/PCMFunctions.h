#pragma once

#include "PortAudio.h"
#include "DR/dr_helper.h"
#include <cstdint>
#include <algorithm>
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

		#define DELAY_BYFFER_SIZE 5 // In Seconds
		struct DelayBuffer
		{
			uint64_t size{ 0 };
			uint64_t endIndex{ 0 };
			float* data{nullptr}; // This functions like a ring buffer.

			void init(uint64_t sampleRate);
			void clear();
			void destroy();
			void setDelay(double seconds);

			/*
				Get oldest sample.
			*/
			float get() const;
			
			/*
				Add new sample to the start.
			*/
			void add(float v);

		private:
			void next();

			uint64_t start{0};
			uint64_t end{2};
			uint64_t sampleRate{0};
		};

	public:
		enum class Func
		{
			NORMAL,
			DISTANCE,
			ECHO
		};

		struct UserData
		{
			ym::SoundHandle handle;
			bool finished{ false };
			PaSampleFormat sampleFormat{ paFloat32 };

			float volume{ 1.f };
			bool loop{ false };

			// Distance
			glm::vec3 sourcePos;
			glm::vec3 receiverPos;
			//glm::vec3 receiverDir;
			glm::vec3 receiverLeft;
			glm::vec3 receiverUp;

			// Echo effect
			DelayBuffer delayBuffer;
			float echoDelay{0.5f};
			float echoGain{0.5f};
		};

		static PaStreamCallback* getCallbackFunction(Func function);

		static void setPos(SoundHandle* handle, uint64_t newPos);

		static int paCallbackNormalPCM(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
			const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData);
		static int paCallbackDistancePCM(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
			const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData);

		static int paCallbackEchoPCM(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
			const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData);

		static OutputData* echoFilter(UserData* data, OutputData* outputData, uint64_t framesRead);

	private:
		static uint64_t readPCMFrames(SoundHandle* handle, uint64_t framesToRead, PaSampleFormat format, void* outBuffer);
		static uint64_t readPCMFramesEffect(SoundHandle* handle, uint64_t framesToRead, PaSampleFormat format, void* outBuffer);
		static float getSample(UserData * data, OutputData * outputData);
		static OutputData getOutputData(UserData* data, void* out);
		static void applyToEar(UserData* data, OutputData* outputData, float value);
		static uint64_t readPCM(UserData* data, uint64_t framesPerBuffer, void* outBuffer);
		static int finish(UserData* data, uint64_t framesRead, uint64_t framesPerBuffer, bool continueFlag);

	};
}
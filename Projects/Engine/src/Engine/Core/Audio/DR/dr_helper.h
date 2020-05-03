#pragma once

#include "dr_mp3.h"
#include "dr_wav.h"

namespace ym
{
	struct SoundHandle
	{
		enum Type { TYPE_NON, TYPE_MP3, TYPE_WAV };
		Type type{ TYPE_NON };

		bool asEffect{ true };
		uint64_t pos{ 0 };

		// Used for streams.
		drmp3 mp3; 
		drwav wav;

		// Used for effects.
		float* directDataF32{ nullptr };
		int16_t* directDataI16{ nullptr };
		uint32_t nChannels{ 0 };
		uint32_t sampleRate{ 0 };
		uint64_t totalFrameCount{ 0 };
	};
}
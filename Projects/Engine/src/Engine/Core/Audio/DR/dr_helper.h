#pragma once

#include "dr_mp3.h"
#include "dr_wav.h"

namespace ym
{
	struct SoundHandler
	{
		enum Type { TYPE_MP3, TYPE_WAV };
		Type type;
		drmp3 mp3;
		drwav wav;
	};
}
#pragma once

#include "dr_mp3.h"
#include "dr_wav.h"

namespace ym
{
	struct SoundHandle
	{
		enum Type { TYPE_NON, TYPE_MP3, TYPE_WAV };
		Type type{ TYPE_NON };
		drmp3 mp3;
		drwav wav;

		~SoundHandle() {
			if (this->type == TYPE_MP3)
				drmp3_uninit(&this->mp3);
			if (this->type == TYPE_WAV)
				drwav_uninit(&this->wav);
		};
	};
}
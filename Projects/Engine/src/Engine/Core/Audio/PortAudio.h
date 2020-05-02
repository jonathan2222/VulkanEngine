#pragma once

#include <PortAudio/portaudio.h>

#define PORT_AUDIO_CHECK(res, ...) \
	{ \
		PaError r = res; \
		if(r != paNoError) { \
			YM_LOG_ERROR("PortAudio error! ({0}) {1}", r, Pa_GetErrorText(res)); \
			YM_ASSERT(false, __VA_ARGS__); \
		} \
	}

namespace ym
{
	class PortAudio
	{
	public:
		PortAudio();
		~PortAudio();

		void init();
		void destroy();

		PaDeviceIndex getDeviceIndex();
		const PaDeviceInfo* getDeviceInfo();

	private:
		PaDeviceIndex deviceIndex;
		mutable const PaDeviceInfo* deviceInfo;
		PaError errorCode;
	};
}

#include "stdafx.h"
#include "AudioSystem.h"

#include "Sound.h"
#include "PortAudio.h"

#define DR_MP3_IMPLEMENTATION
#include "DR/dr_mp3.h"
#define DR_WAV_IMPLEMENTATION
#include "DR/dr_wav.h"

ym::AudioSystem::AudioSystem()
{
}

ym::AudioSystem::~AudioSystem()
{
}

ym::AudioSystem* ym::AudioSystem::get()
{
	static AudioSystem audioSystem;
	return &audioSystem;
}

#define NUM_SECONDS   (4)
#define SAMPLE_RATE   (44100)

struct paTestData
{
	float sine[200];
	int left_phase;
	int right_phase;
};

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
int patestCallback(const void* inputBuffer, void* outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void* userData)
{
	ym::SoundHandler* data = (ym::SoundHandler*)userData;
	float* out = (float*)outputBuffer;
	unsigned long i, j;

	(void)timeInfo; /* Prevent unused variable warnings. */
	(void)statusFlags;
	(void)inputBuffer;

	ym::AudioSystem::readPCMFrames(data, framesPerBuffer, out);
	/*
	for (i = 0; i < framesPerBuffer; i++)
	{
		*out++ = data->sine[data->left_phase]; 
		*out++ = data->sine[data->right_phase];
		data->left_phase += 1;
		if (data->left_phase >= 200) data->left_phase -= 200;
		data->right_phase += 3;
		if (data->right_phase >= 200) data->right_phase -= 200;
	}*/

	return paContinue;
}

void ym::AudioSystem::init()
{
	this->portAudio = new PortAudio();
	this->portAudio->init();
	YM_LOG_INFO("Initialized audio system.");

	std::string path = YM_ASSETS_FILE_PATH + "/Audio/SoundEffects/ButtonOn.mp3";
	SoundHandler soundHandler;
	loadFile(&soundHandler, path);
	//readPCMFrames(&soundHandler, framesToRead, outBuffer);

	/*paTestData data;
	for (uint32_t i = 0; i < 200; i++)
		data.sine[i] = (float)sin(((double)i / (double)200) * 3.14159265 * 2.);
	data.left_phase = data.right_phase = 0;
	*/
	PaStreamParameters outputParameters;
	outputParameters.device = this->portAudio->getDeviceIndex();
	outputParameters.channelCount = 2;       /* stereo output */
	outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
	outputParameters.suggestedLatency = this->portAudio->getDeviceInfo()->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;

	PaStream* stream;
	PaError err = Pa_OpenStream(
		&stream,
		NULL,          
		&outputParameters,
		SAMPLE_RATE,
		paFramesPerBufferUnspecified,        /* frames per buffer */
		paClipOff,
		patestCallback,
		&soundHandler);
	PORT_AUDIO_CHECK(err, "Failed to open default stream!");

	PORT_AUDIO_CHECK(Pa_StartStream(stream), "Failed to start stream!");

	YM_LOG_INFO("Play for {} seconds.", 4);
	Pa_Sleep(4 * 1000);

	PORT_AUDIO_CHECK(Pa_StopStream(stream), "Failed to stop stream!");
	PORT_AUDIO_CHECK(Pa_CloseStream(stream), "Failed to close stream!");
}

void ym::AudioSystem::destroy()
{
	// Remove all sounds create by the system.
	for (Sound*& sound : this->sounds)
	{
		sound->destroy();
		delete sound;
	}
	this->sounds.clear();

	// Remove all soundStreams create by the system.
	for (Sound*& stream : this->soundStreams)
	{
		stream->destroy();
		delete stream;
	}
	this->soundStreams.clear();

	this->portAudio->destroy();
	SAFE_DELETE(this->portAudio);

	YM_LOG_INFO("Destroyed audio system.");
}

void ym::AudioSystem::update()
{
	
}

ym::Sound* ym::AudioSystem::createSound(const std::string& filePath)
{
	/*Sound* sound = new Sound(this->system);
	FMOD_CHECK(this->system->createSound(filePath.c_str(), FMOD_DEFAULT, 0, &sound->sound), "Failed to create sound {}!", filePath.c_str());
	FMOD_CHECK(sound->sound->setMode(FMOD_LOOP_OFF), "Failed to set sound mode for sound {}!", filePath.c_str());
	sound->init();
	this->sounds.push_back(sound);
	return sound;*/
	return nullptr;
}

void ym::AudioSystem::removeSound(Sound* sound)
{
	sound->destroy();
	this->sounds.erase(std::remove(this->sounds.begin(), this->sounds.end(), sound), this->sounds.end());
	delete sound;
}

ym::Sound* ym::AudioSystem::createStream(const std::string& filePath)
{
	/*Sound* stream = new Sound(this->system);
	FMOD_CHECK(this->system->createStream(filePath.c_str(), FMOD_DEFAULT, nullptr, &stream->sound), "Failed to create sound stream {}!", filePath.c_str());
	FMOD_CHECK(stream->sound->setMode(FMOD_LOOP_NORMAL), "Failed to set sound mode for sound stream {}!", filePath.c_str());
	stream->init();
	this->soundStreams.push_back(stream);
	return stream;*/
	return nullptr;
}

void ym::AudioSystem::removeStream(Sound* soundStream)
{
	soundStream->destroy();
	this->soundStreams.erase(std::remove(this->soundStreams.begin(), this->soundStreams.end(), soundStream), this->soundStreams.end());
	delete soundStream;
}

void ym::AudioSystem::loadFile(SoundHandler* soundHandler, const std::string& filePath)
{
	// Get file type.
	std::string ext = filePath.substr(filePath.find_last_of(".") + 1);
	if (ext == "mp3") soundHandler->type = SoundHandler::Type::TYPE_MP3;
	if (ext == "wav") soundHandler->type = SoundHandler::Type::TYPE_WAV;

	// Load file.
	switch (soundHandler->type)
	{
	case SoundHandler::Type::TYPE_MP3:
		{
			YM_ASSERT(drmp3_init_file(&soundHandler->mp3, filePath.c_str(), NULL), "Failed to load sound %s of type mp3!", filePath.c_str());
		}
		break;
	case SoundHandler::Type::TYPE_WAV:
	{
		YM_ASSERT(drwav_init_file(&soundHandler->wav, filePath.c_str(), NULL), "Failed to load sound %s of type wav!", filePath.c_str());
	}
		break;
	default:
		YM_ASSERT(false, "Failed to load sound %s, unrecognized file type!", filePath.c_str());
		break;
	}
	
}

uint64_t ym::AudioSystem::readPCMFrames(SoundHandler* soundHandler, uint64_t framesToRead, float* outBuffer)
{
	drmp3_uint64 ftr = (drmp3_uint64)framesToRead;
	switch (soundHandler->type)
	{
	case SoundHandler::Type::TYPE_MP3:
		return drmp3_read_pcm_frames_f32(&soundHandler->mp3, ftr, outBuffer);
		break;
	case SoundHandler::Type::TYPE_WAV:
		return drwav_read_pcm_frames_f32(&soundHandler->wav, ftr, outBuffer);
		break;
	default:
		return 0;
		break;
	}
}

uint64_t ym::AudioSystem::readPCMFrames(SoundHandler* soundHandler, uint64_t framesToRead, int16_t* outBuffer)
{
	drmp3_uint64 ftr = (drmp3_uint64)framesToRead;
	switch (soundHandler->type)
	{
	case SoundHandler::Type::TYPE_MP3:
		return drmp3_read_pcm_frames_s16(&soundHandler->mp3, ftr, outBuffer);
		break;
	case SoundHandler::Type::TYPE_WAV:
		return drwav_read_pcm_frames_s16(&soundHandler->wav, ftr, outBuffer);
		break;
	default:
		return 0;
		break;
	}
}

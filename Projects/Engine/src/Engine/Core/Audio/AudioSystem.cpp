#include "stdafx.h"
#include "AudioSystem.h"

#include "Sound.h"

#define DR_MP3_IMPLEMENTATION
#include "DR/dr_mp3.h"
#define DR_WAV_IMPLEMENTATION
#include "DR/dr_wav.h"

#include "PCMFunctions.h"

// For debug settings.
#include "Utils/Imgui/imgui.h"
#include "Engine/Core/Audio/Filters/DistanceFilter.h"
#include "Engine/Core/Audio/Filters/EchoFilter.h"
#include "Engine/Core/Audio/Filters/LowpassFilter.h"
#include "Engine/Core/Audio/Filters/HighpassFilter.h"

#include "Utils/Utils.h"

ym::AudioSystem::AudioSystem() : portAudio(nullptr)
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

void ym::AudioSystem::init()
{
	this->portAudio = new PortAudio();
	this->portAudio->init();
	YM_LOG_INFO("Initialized audio system.");
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

void ym::AudioSystem::setMasterVolume(float volume)
{
	this->masterVolume = volume;
}

void ym::AudioSystem::setStreamVolume(float volume)
{
	this->streamVolume = volume;
}

void ym::AudioSystem::setEffectsVolume(float volume)
{
	this->effectsVolume = volume;
}

ym::Sound* ym::AudioSystem::createSound(const std::string& filePath)
{
	Sound* sound = new Sound(this->portAudio);
	PCM::UserData* userData = new PCM::UserData();
	userData->finished = false;
	userData->sampleFormat = paFloat32;
	loadEffectFile(&userData->handle, filePath, userData->sampleFormat);
	sound->init(userData);
	sound->setName(Utils::getNameFromPath(filePath));
	this->sounds.push_back(sound);
	return sound;
}

void ym::AudioSystem::removeSound(Sound* sound)
{
	sound->destroy();
	this->sounds.erase(std::remove(this->sounds.begin(), this->sounds.end(), sound), this->sounds.end());
	delete sound;
}

ym::Sound* ym::AudioSystem::createStream(const std::string& filePath)
{
	Sound* stream = new Sound(this->portAudio);
	PCM::UserData* userData = new PCM::UserData();
	userData->finished = false;
	userData->sampleFormat = paFloat32;
	userData->soundData.loop = true;
	loadStreamFile(&userData->handle, filePath);
	stream->init(userData);
	stream->setName(Utils::getNameFromPath(filePath));
	this->soundStreams.push_back(stream);
	return stream;
}

void ym::AudioSystem::removeStream(Sound* soundStream)
{
	soundStream->destroy();
	this->soundStreams.erase(std::remove(this->soundStreams.begin(), this->soundStreams.end(), soundStream), this->soundStreams.end());
	delete soundStream;
}

void ym::AudioSystem::loadStreamFile(SoundHandle* soundHandle, const std::string& filePath)
{
	soundHandle->asEffect = false;

	// Get file type.
	std::string ext = filePath.substr(filePath.find_last_of(".") + 1);
	if (ext == "mp3") soundHandle->type = SoundHandle::Type::TYPE_MP3;
	if (ext == "wav") soundHandle->type = SoundHandle::Type::TYPE_WAV;

	// Load file.
	switch (soundHandle->type)
	{
	case SoundHandle::Type::TYPE_MP3:
		{
			YM_ASSERT(drmp3_init_file(&soundHandle->mp3, filePath.c_str(), NULL), "Failed to load sound {} of type mp3!", filePath.c_str());
			soundHandle->nChannels = soundHandle->mp3.channels;
			soundHandle->sampleRate = soundHandle->mp3.sampleRate;
		}
		break;
	case SoundHandle::Type::TYPE_WAV:
		{
			YM_ASSERT(drwav_init_file(&soundHandle->wav, filePath.c_str(), NULL), "Failed to load sound {} of type wav!", filePath.c_str());
			soundHandle->nChannels = soundHandle->wav.channels;
			soundHandle->sampleRate = soundHandle->wav.sampleRate;
		}
		break;
	default:
		YM_ASSERT(false, "Failed to load sound {}, unrecognized file type!", filePath.c_str());
		break;
	}
}

void ym::AudioSystem::loadEffectFile(SoundHandle* soundHandle, const std::string& filePath, PaSampleFormat format)
{
	soundHandle->asEffect = true;

	// Get file type.
	std::string ext = filePath.substr(filePath.find_last_of(".") + 1);
	if (ext == "mp3") soundHandle->type = SoundHandle::Type::TYPE_MP3;
	if (ext == "wav") soundHandle->type = SoundHandle::Type::TYPE_WAV;

	// Load file.
	switch (soundHandle->type)
	{
	case SoundHandle::Type::TYPE_MP3:
	{
		drmp3_config config = {};
		if(format == paFloat32)
			soundHandle->directDataF32 = drmp3_open_file_and_read_pcm_frames_f32(filePath.c_str(), &config, &soundHandle->totalFrameCount, NULL);
		else if (format == paInt16)
			soundHandle->directDataI16 = drmp3_open_file_and_read_pcm_frames_s16(filePath.c_str(), &config, &soundHandle->totalFrameCount, NULL);
		soundHandle->nChannels = config.channels;
		soundHandle->sampleRate = config.sampleRate;
	}
	break;
	case SoundHandle::Type::TYPE_WAV:
	{
		drmp3_config config = {};
		if (format == paFloat32)
			soundHandle->directDataF32 = drwav_open_file_and_read_pcm_frames_f32(filePath.c_str(), &soundHandle->nChannels, &soundHandle->sampleRate, &soundHandle->totalFrameCount, NULL);
		if (format == paInt16)
			soundHandle->directDataI16 = drwav_open_file_and_read_pcm_frames_s16(filePath.c_str(), &soundHandle->nChannels, &soundHandle->sampleRate, &soundHandle->totalFrameCount, NULL);
		YM_ASSERT(drwav_init_file(&soundHandle->wav, filePath.c_str(), NULL), "Failed to load sound {} of type wav!", filePath.c_str());
	}
	break;
	default:
		YM_ASSERT(false, "Failed to load sound {}, unrecognized file type!", filePath.c_str());
		break;
	}
}

void ym::AudioSystem::drawAudioSettings()
{
	{
		static bool my_tool_active = true;
		ImGui::Begin("Audio settings", &my_tool_active, ImGuiWindowFlags_MenuBar);
		ImGui::SliderFloat("Master volume", &this->masterVolume, 0.0f, 1.f, "%.3f");

		uint32_t index = 0;
		if (ImGui::CollapsingHeader("Effects"))
		{
			ImGui::SliderFloat("Effects volume", &this->effectsVolume, 0.0f, 1.f, "%.3f");
			for (Sound* sound : this->sounds)
			{
				sound->setGroupVolume(this->effectsVolume);
				sound->setMasterVolume(this->masterVolume);
				drawSoundSettings(sound, index);
				index++;
			}
		}

		if (ImGui::CollapsingHeader("Streams"))
		{
			ImGui::SliderFloat("Streams Volume", &this->streamVolume, 0.0f, 1.f, "%.3f");
			for (Sound* sound : this->soundStreams)
			{
				sound->setGroupVolume(this->streamVolume);
				sound->setMasterVolume(this->masterVolume);
				drawSoundSettings(sound, index);
				index++;
			}
		}

		ImGui::End();
	}
}

void ym::AudioSystem::drawSoundSettings(Sound* sound, uint32_t index)
{
	std::string name = "[" + std::to_string(index) + "]" + sound->getName();
	if (ImGui::TreeNode(name.c_str()))
	{
		float volume = sound->getVolume();
		ImGui::SliderFloat("Volume", &volume, 0.0f, 1.f, "%.3f");
		sound->setVolume(volume);

		auto& filters = sound->getFilters();
		for (ym::Filter* filter : filters)
		{
			if (ImGui::CollapsingHeader(filter->getName().c_str()))
			{
				if (ym::LowpassFilter * lowpassF = dynamic_cast<ym::LowpassFilter*>(filter))
				{
					float fcLow = lowpassF->getCutoffFrequency();
					ImGui::SliderFloat("[Low]Cutoff frequency", &fcLow, 0.0f, lowpassF->getSampleRate() * 0.5f, "%.0f Hz");
					lowpassF->setCutoffFrequency(fcLow);
				}

				if (ym::HighpassFilter * highpassF = dynamic_cast<ym::HighpassFilter*>(filter))
				{
					float fcHigh = highpassF->getCutoffFrequency();
					ImGui::SliderFloat("[High]Cutoff frequency", &fcHigh, 0.0f, highpassF->getSampleRate() * 0.5f, "%.0f Hz");
					highpassF->setCutoffFrequency(fcHigh);
				}

				if (ym::EchoFilter * echoF = dynamic_cast<ym::EchoFilter*>(filter))
				{
					float gain = echoF->getGain();
					ImGui::SliderFloat("Gain", &gain, 0.0f, 1.f, "%.3f");
					echoF->setGain(gain);
					float delay = echoF->getDelay();
					ImGui::SliderFloat("Delay", &delay, 0.0f, (float)MAX_CIRCULAR_BUFFER_SIZE, "%.3f sec");
					echoF->setDelay(delay);
				}

				if (ym::DistanceFilter * distF = dynamic_cast<ym::DistanceFilter*>(filter))
				{
					ImGui::Text("Uses factor = 1/distance^2 for sound attenuation.");
					glm::vec3 pos = sound->getPosition();
					ImGui::Text("Source position: (%f, %f, %f)", pos.x, pos.y, pos.z);
				}
			}
		}
		ImGui::TreePop();
	}
}

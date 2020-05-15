#include "SandboxLayer.h"

#include <glTF/stb_image.h>

#include "Engine/Core/Scene/GLTFLoader.h"
#include "Engine/Core/Graphics/Renderer.h"
#include "Engine/Core/Display/Display.h"
#include "Engine/Core/Input/Input.h"
#include "Engine/Core/Audio/AudioSystem.h"
#include "Engine/Core/Scene/ObjectManager.h"

#include "Engine/Core/Audio/Filters/DistanceFilter.h"
#include "Engine/Core/Audio/Filters/EchoFilter.h"
#include "Engine/Core/Audio/Filters/LowpassFilter.h"

void SandboxLayer::onStart(ym::Renderer* renderer)
{
	ym::GLTFLoader::loadOnThread(YM_ASSETS_FILE_PATH + "Models/Tree/Tree.glb", &this->treeModel);
	//ym::GLTFLoader::loadOnThread(YM_ASSETS_FILE_PATH + "Models/Gun/Cerberus.glb", &this->cubeModel);
	ym::GLTFLoader::loadOnThread(YM_ASSETS_FILE_PATH + "Models/Cube/Cube.gltf", &this->cubeModel);
	ym::GLTFLoader::loadOnThread(YM_ASSETS_FILE_PATH + "Models/WaterBottle/WaterBottle.glb", &this->waterBottleModel);
	ym::GLTFLoader::loadOnThread(YM_ASSETS_FILE_PATH + "Models/Scene1/Chest.glb", &this->chestModel);
	ym::GLTFLoader::loadOnThread(YM_ASSETS_FILE_PATH + "Models/Scene1/Terrain2.glb", &this->terrain2Model);
	ym::GLTFLoader::loadOnThread(YM_ASSETS_FILE_PATH + "Models/Scene1/Castle.glb", &this->fortModel);
	ym::GLTFLoader::loadOnThread(YM_ASSETS_FILE_PATH + "Models/Scene1/WoodenCrate.glb", &this->woodenCrateModel);
	//ym::GLTFLoader::loadOnThread(YM_ASSETS_FILE_PATH + "Models/Sponza/glTF/Sponza.gltf", &this->sponzaModel);

	int width, height;
	int channels;
	std::string path = YM_ASSETS_FILE_PATH + "/Textures/Heightmaps/australia.jpg";
	unsigned char* data = static_cast<unsigned char*>(stbi_load(path.c_str(), &width, &height, &channels, 1));
	if (data == nullptr)
		YM_LOG_ERROR("Failed to load terrain, couldn't find file!");
	else {
		float scale = 2.0f;
		ym::Terrain::Description terrainDescription = {};
		terrainDescription.vertDist = scale;
		terrainDescription.minZ = 0.0f;
		terrainDescription.maxZ = 20.0f;
		terrainDescription.origin = glm::vec3(-(width / 2.f) * scale, 0.f, -(height / 2.f) * scale);
		this->terrain.init(width, height, data, terrainDescription);
		YM_LOG_INFO("Loaded terrain: {} successfully!", path);
		delete[] data;
	}

	float aspect = ym::Display::get()->getAspectRatio();
	this->camera.init(aspect, 45.f, { 0.f, 2, 0.f }, { 0.f, 2, 1.f }, 1.0f, 10.0f);

	renderer->setActiveCamera(&this->camera);

	this->cameraLockSound = ym::AudioSystem::get()->createSound(YM_ASSETS_FILE_PATH + "/Audio/SoundEffects/ButtonOff.mp3");
	this->cameraLockSound->setVolume(0.5f);
	this->cameraUnlockSound = ym::AudioSystem::get()->createSound(YM_ASSETS_FILE_PATH + "/Audio/SoundEffects/ButtonOn.mp3");
	this->cameraUnlockSound->setVolume(0.5f);
	
	this->ambientSound = ym::AudioSystem::get()->createStream(YM_ASSETS_FILE_PATH + "/Audio/Ambient/Rainforest.mp3");
	this->ambientSound->play();
	this->ambientSound->setVolume(0.02f);

	this->music = ym::AudioSystem::get()->createSound(YM_ASSETS_FILE_PATH + "/Audio/Music/MedievalMusic.mp3");
	this->music->setVolume(0.02f);
	this->music->play();
	//this->music = ym::AudioSystem::get()->createStream(YM_ASSETS_FILE_PATH + "/Audio/Music/DunnoJBPet.mp3", ym::PCM::Func::NORMAL);

	// Cube
	glm::mat4 transformCube(0.5f);
	transformCube[3][3] = 1.0f;
	transformCube = glm::translate(glm::mat4(1.0f), { 3.0f, 1.f, 2.f }) * transformCube;
	this->cubeObject = ym::ObjectManager::get()->createGameObject(transformCube, &this->cubeModel);

	// Add chest
	glm::mat4 transformChest(1.0f);
	transformChest = glm::translate(glm::mat4(1.0f), { -4.0f, 1.f, -4.f }) * transformChest;
	this->chestObject = ym::ObjectManager::get()->createGameObject(transformChest, &this->chestModel);
	this->pokerChips = ym::AudioSystem::get()->createStream(YM_ASSETS_FILE_PATH + "/Audio/SoundEffects/MovieStart.mp3");
	this->pokerChips->setLoop(true);
	this->pokerChips->addFilter(new ym::EchoFilter());
	this->pokerChips->addFilter(new ym::DistanceFilter());
	this->pokerChips->addFilter(new ym::LowpassFilter());
	this->pokerChips->setVolume(0.4f);
	this->pokerChips->play();

	//ym::ObjectManager::get()->createGameObject(glm::mat4(1.f), &this->terrain2Model);
	this->fortObject = ym::ObjectManager::get()->createGameObject(glm::mat4(1.f), &this->fortModel);

	glm::mat4 transformCrate(1.0f);
	transformCrate = glm::translate(glm::mat4(1.0f), { -3.0f, 1.f, 2.f }) * transformCrate;
	this->woodenCrateObject = ym::ObjectManager::get()->createGameObject(transformCrate, &this->woodenCrateModel);

	// TODO: The scale can be changed to only set its distance to max in the shader instead!
	this->cubeMap.init(100.f, YM_ASSETS_FILE_PATH + "/Textures/skybox/");

	/*
	glm::mat4 transformSponza(1.0f);
	transformSponza[3][3] = 1.0f;
	transformSponza = glm::translate(glm::mat4(1.0f), { 0.0f, 1.f, 0.f }) * transformSponza;
	this->sponzaObject = ym::ObjectManager::get()->createGameObject(transformSponza, &this->sponzaModel);
	*/
}

void SandboxLayer::onUpdate(float dt)
{
	ym::Input* input = ym::Input::get();
	static bool lock = false;
	ym::KeyState keyState = input->getKeyState(ym::Key::C);
	if (keyState == ym::KeyState::FIRST_RELEASED)
	{
		lock ^= 1;
		YM_LOG_WARN("Toggle Camera");
		if (lock) 
		{
			input->centerMouse();
			input->lockMouse();
			this->cameraLockSound->play();
		}
		else
		{
			input->unlockMouse();
			this->cameraUnlockSound->play();
		}
	}

	if (lock)
	{
		input->centerMouse();
		this->camera.update(dt);
	}

	// Quit if ESCAPE is pressed.
	if (input->isKeyPressed(ym::Key::ESCAPE))
		this->terminate();

	if (input->getKeyState(ym::Key::KP_1) == ym::KeyState::FIRST_PRESSED)
	{
		this->music->play();
		//this->music->setVolume(0.2f);
		YM_LOG_INFO("Play sound");
	}

	if (input->getKeyState(ym::Key::KP_2) == ym::KeyState::FIRST_PRESSED)
	{
		this->music->pause();
		YM_LOG_INFO("Pause sound");
	}

	if (input->getKeyState(ym::Key::KP_3) == ym::KeyState::FIRST_PRESSED)
	{
		this->music->stop();
		YM_LOG_INFO("Stop sound");
	}

	if (input->getKeyState(ym::Key::KP_ADD) == ym::KeyState::FIRST_PRESSED)
	{
		this->music->applyVolume(0.05f);
		YM_LOG_INFO("Changed volume to {}", this->music->getVolume());
	}

	if (input->getKeyState(ym::Key::KP_SUBTRACT) == ym::KeyState::FIRST_PRESSED)
	{
		this->music->applyVolume(-0.05f);
		YM_LOG_INFO("Changed volume to {}", this->music->getVolume());
	}

	// ---------- Update game objects ----------

	// Add tree objects when pressing E
	static int32_t maxTrees = 15;
	static float r = maxTrees;
	keyState = input->getKeyState(ym::Key::E);
	if (keyState == ym::KeyState::FIRST_RELEASED)
		r = ++maxTrees;
	for (int32_t i = 0; i < maxTrees; i++)
	{
		float angle = (float)i / (float)maxTrees * 2 * glm::pi<float>();
		float x = glm::cos(angle) * r;
		float z = glm::sin(angle) * r;
		auto transform = glm::mat4(1.0f);
		transform = glm::translate(glm::mat4(1.0f), { x, 0.f, z });

		if (i >= this->treesObjects.size())
			this->treesObjects.push_back(ym::ObjectManager::get()->createGameObject(transform, &this->treeModel));
		else
			this->treesObjects[i]->setTransform(transform);
	}

	// Toggle Water bottle object when pressing F
	static bool spawn = false;
	keyState = input->getKeyState(ym::Key::F);
	if (keyState == ym::KeyState::FIRST_RELEASED)
	{
		spawn ^= 1;
		if (spawn)
		{
			auto transform = glm::mat4(10.0f);
			transform[3][3] = 1.0f;
			transform = glm::translate(glm::mat4(1.0f), { 0.0f, 2.f, 0.f }) * transform;
			this->watterBottleObject = ym::ObjectManager::get()->createGameObject(transform, &this->waterBottleModel);
		}
		else
		{
			ym::ObjectManager::get()->removeGameObject(this->watterBottleObject);
		}
	}

	// Update sound user data.
	this->pokerChips->setSourcePosition(this->chestObject->getPos());
	this->pokerChips->setReceiverPosition(this->camera.getPosition());
	this->pokerChips->setReceiverLeft(-this->camera.getRight());
	this->pokerChips->setReceiverUp(-this->camera.getUp());

	static float t = 0.f;
	t += dt;
	if (t > 3.0f)
	{
		//this->pokerChips->play();
		t = 0;
	}
	//this->pokerChips->setReceiverDir(-this->camera.getDirection());
}

void SandboxLayer::onRender(ym::Renderer* renderer)
{
	renderer->begin();

	//ImGui::ShowDemoWindow();

	{
		static bool my_tool_active = true;
		ImGui::Begin("Audio settings", &my_tool_active, ImGuiWindowFlags_MenuBar);

		ym::Sound* source = this->pokerChips;

		float volume = source->getVolume();
		ImGui::SliderFloat("Volume", &volume, 0.0f, 1.f, "%.01f");
		source->setVolume(volume);

		auto& filters = source->getFilters();
		for (ym::Filter* filter : filters)
		{
			if (ImGui::CollapsingHeader(filter->getName().c_str()))
			{
				if (ym::LowpassFilter * lowpassF = dynamic_cast<ym::LowpassFilter*>(filter))
				{
					float fc = lowpassF->getCutoffFrequency();
					ImGui::SliderFloat("Cutoff frequency", &fc, 0.0f, lowpassF->getSampleRate()*0.5f, "%.0f Hz");
					lowpassF->setCutoffFrequency(fc);
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

				if (ym::DistanceFilter * distancF = dynamic_cast<ym::DistanceFilter*>(filter))
				{
					ImGui::Text("Nothing to change here");
				}
			}
		}
		
		ImGui::End();
	}

	glm::mat4 transform(1.f);
	renderer->drawCubeMap(&this->cubeMap, transform);
	renderer->drawAllModels(ym::ObjectManager::get());

	//renderer->drawTerrain(&this->terrain, glm::mat4(1.0f));

	renderer->end();
}

void SandboxLayer::onRenderImGui()
{
}

void SandboxLayer::onQuit()
{
	ym::Input::get()->unlockMouse();
	this->treeModel.destroy();
	this->cubeModel.destroy();
	this->waterBottleModel.destroy();
	this->sponzaModel.destroy();
	this->chestModel.destroy();
	this->woodenCrateModel.destroy();
	this->fortModel.destroy();
	this->terrain2Model.destroy();
	this->cubeMap.destroy();
	this->terrain.destroy();
	this->camera.destroy();
}

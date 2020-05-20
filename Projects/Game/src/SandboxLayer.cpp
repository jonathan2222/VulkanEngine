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
#include "Engine/Core/Audio/Filters/HighpassFilter.h"

#include "Engine/Core/Vulkan/Factory.h"
#include "Utils/Utils.h"

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
	ym::GLTFLoader::loadOnThread(YM_ASSETS_FILE_PATH + "Models/MetalBalls/MetalBalls.glb", &this->metalBallsModel);
	ym::GLTFLoader::loadOnThread(YM_ASSETS_FILE_PATH + "Models/Scene1/KnightSword.glb", &this->knightSwordModel);
	ym::GLTFLoader::loadOnThread(YM_ASSETS_FILE_PATH + "Models/Scene1/KnightSpear.glb", &this->knightSpearModel);
	ym::GLTFLoader::loadOnThread(YM_ASSETS_FILE_PATH + "Models/Scene1/Dragon.glb", &this->dragonModel);
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
	this->cameraLockSound->setVolume(0.1f);
	this->cameraUnlockSound = ym::AudioSystem::get()->createSound(YM_ASSETS_FILE_PATH + "/Audio/SoundEffects/ButtonOn.mp3");
	this->cameraUnlockSound->setVolume(0.1f);
	
	this->ambientSound = ym::AudioSystem::get()->createStream(YM_ASSETS_FILE_PATH + "/Audio/Ambient/Rainforest.mp3");
	this->ambientSound->play();
	this->ambientSound->setVolume(0.015f);

	this->music = ym::AudioSystem::get()->createSound(YM_ASSETS_FILE_PATH + "/Audio/Music/MedievalMusic.mp3");
	this->music->setVolume(0.01f);
	this->music->setLoop(true);
	this->music->addFilter(new ym::LowpassFilter());
	this->music->play();
	//this->music = ym::AudioSystem::get()->createStream(YM_ASSETS_FILE_PATH + "/Audio/Music/DunnoJBPet.mp3", ym::PCM::Func::NORMAL);

	// Cube
	glm::mat4 transformCube(0.5f);
	transformCube[3][3] = 1.0f;
	transformCube = glm::translate(glm::mat4(1.0f), { 3.0f, 1.f, 2.f }) * transformCube;
	this->cubeObject = ym::ObjectManager::get()->createGameObject(transformCube, &this->cubeModel);

	transformCube = glm::translate(glm::mat4(1.0f), { -3.0f, 7.f, 2.f });
	ym::ObjectManager::get()->createGameObject(transformCube, &this->metalBallsModel);

	// Add chest
	glm::mat4 transformChest(1.0f);
	transformChest = glm::rotate(glm::mat4(1.f), glm::pi<float>() / 4.f, { 0.f, 1.f, 0.f });
	transformChest = glm::translate(glm::mat4(1.0f), { -4.0f, 0.f, 3.f }) * transformChest;
	this->chestObject = ym::ObjectManager::get()->createGameObject(transformChest, &this->chestModel);
	this->chestSound = ym::AudioSystem::get()->createStream(YM_ASSETS_FILE_PATH + "Audio/SoundEffects/ButtonOff.mp3");
	this->chestSound->setLoop(true);
	this->chestSound->addFilter(new ym::EchoFilter());
	this->chestSound->addFilter(new ym::DistanceFilter());
	this->chestSound->addFilter(new ym::LowpassFilter());
	this->chestSound->setVolume(0.4f);
	this->chestSound->play();

	//ym::ObjectManager::get()->createGameObject(glm::mat4(1.f), &this->terrain2Model);
	this->fortObject = ym::ObjectManager::get()->createGameObject(glm::mat4(1.f), &this->fortModel);

	glm::mat4 transformCrate(1.0f);
	transformCrate = glm::translate(glm::mat4(1.0f), { 3.0f, 0.f, 2.f }) * transformCrate;
	this->woodenCrateObject = ym::ObjectManager::get()->createGameObject(transformCrate, &this->woodenCrateModel);
	this->crateSound = ym::AudioSystem::get()->createSound(YM_ASSETS_FILE_PATH + "Audio/SoundEffects/SlidingDoor.mp3");
	this->crateSound->setLoop(false);
	this->crateSound->addFilter(new ym::DistanceFilter());
	this->crateSound->addFilter(new ym::LowpassFilter());
	this->crateSound->setVolume(0.4f);

	this->crowdSound = ym::AudioSystem::get()->createSound(YM_ASSETS_FILE_PATH + "Audio/Ambient/crowd.mp3");
	this->crowdSound->setSourcePosition({ -2.f, 1.f, -5.f });
	this->crowdSound->setLoop(true);
	this->crowdSound->addFilter(new ym::DistanceFilter());
	this->crowdSound->addFilter(new ym::LowpassFilter());
	this->crowdSound->setVolume(0.1f);
	this->crowdSound->play();

	{ // Dragon
		glm::mat4 transform(1.f);
		transform = glm::translate(glm::mat4(1.0f), { 2.0f, 0.f, 0.f }) * glm::rotate(glm::mat4(1.f), 0.f, { 0.f, 1.f, 0.f });
		ym::GameObject* dragon = ym::ObjectManager::get()->createGameObject(transform, &this->dragonModel);
	}

	{ // Knight Sword
		glm::mat4 transform(1.f);
		transform = glm::translate(glm::mat4(1.0f), { 0.0f, 0.f, -7.f }) * glm::rotate(glm::mat4(1.f), 0.f, { 0.f, 1.f, 0.f });
		ym::GameObject* knightSword = ym::ObjectManager::get()->createGameObject(transform, &this->knightSwordModel);
		this->helloSound1 = ym::AudioSystem::get()->createSound(YM_ASSETS_FILE_PATH + "Audio/SoundEffects/Hello.mp3");
		glm::vec3 soundPos = knightSword->getPos() + glm::vec3(0.f, 1.6f, 0.f);
		this->helloSound1->setSourcePosition(soundPos);
		this->helloSound1->setLoop(true);
		this->helloSound1->addFilter(new ym::DistanceFilter());
		this->helloSound1->addFilter(new ym::LowpassFilter());
		this->helloSound1->setVolume(0.4f);
		this->helloSound1->play();
	}

	{ // Knight Spear
		glm::mat4 transform(1.f);
		transform = glm::translate(glm::mat4(1.0f), { 0.0f, 0.f, 7.f }) * glm::rotate(glm::mat4(1.f), glm::pi<float>(), { 0.f, 1.f, 0.f });
		ym::GameObject* knightSpear = ym::ObjectManager::get()->createGameObject(transform, &this->knightSpearModel);
		this->helloSound2 = ym::AudioSystem::get()->createSound(YM_ASSETS_FILE_PATH + "Audio/SoundEffects/Hello.mp3");
		glm::vec3 soundPos = knightSpear->getPos() + glm::vec3(0.f, 1.6f, 0.f);
		this->helloSound2->setSourcePosition(soundPos);
		this->helloSound2->setLoop(true);
		ym::EchoFilter* filter = new ym::EchoFilter();
		this->helloSound2->addFilter(filter);
		filter->setDelay(0.8f);
		this->helloSound2->addFilter(new ym::DistanceFilter());
		this->helloSound2->addFilter(new ym::LowpassFilter());
		this->helloSound2->setVolume(0.4f);
		this->helloSound2->play();
	}

	/*
	glm::mat4 transformSponza(1.0f);
	transformSponza[3][3] = 1.0f;
	transformSponza = glm::translate(glm::mat4(1.0f), { 0.0f, 1.f, 0.f }) * transformSponza;
	this->sponzaObject = ym::ObjectManager::get()->createGameObject(transformSponza, &this->sponzaModel);
	*/

	this->environmentMap = renderer->getDefaultEnvironmentMap();

	/*
	const uint32_t N = 4;
	float test[N];
	//std::complex<float> cArr[N];
	for (uint32_t i = 0; i < N; i++) test[i] = i;
	auto cArr = ym::Utils::fft2(test, N, 1);
	for (uint32_t i = 0; i < N; i++)
		YM_LOG_WARN("FFT[{}] = {} + {}i", i, cArr[i].real(), cArr[i].imag());
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
	static float r = (float)maxTrees;
	keyState = input->getKeyState(ym::Key::E);
	if (keyState == ym::KeyState::FIRST_RELEASED)
		r = (float)(++maxTrees);
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
	this->chestSound->setSourcePosition(this->chestObject->getPos());
	this->chestSound->setReceiver(&this->camera);

	this->crateSound->setSourcePosition(this->woodenCrateObject->getPos());
	this->crateSound->setReceiver(&this->camera);

	this->crowdSound->setReceiver(&this->camera);
	this->helloSound1->setReceiver(&this->camera);
	this->helloSound2->setReceiver(&this->camera);

	static float t = 0.f;
	t += dt;
	if (t > 3.0f)
	{
		this->crateSound->play();
		t = 0;
	}
}

void SandboxLayer::onRender(ym::Renderer* renderer)
{
	renderer->begin();

	//ImGui::ShowDemoWindow();

	ym::AudioSystem::get()->drawAudioSettings();

	{
		static bool my_tool_active = true;
		static float screenExposure = 4.5f;
		static float screenGamma = 2.2f;
		ImGui::Begin("Screen Settings", &my_tool_active);
		ImGui::SliderFloat("Screen Exposure", &screenExposure, 0.1f, 10.f, "%.001f");
		ImGui::SliderFloat("Screen Gamma", &screenGamma, 0.1f, 4.0f, "%.01f");
		renderer->setScreenData(screenExposure, screenGamma);
		ImGui::End();
	}
	// 360 - 500 hög order.

	glm::mat4 transform(1.f);
	renderer->drawSkybox(this->environmentMap);
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
	this->metalBallsModel.destroy();

	this->knightSpearModel.destroy();
	this->knightSwordModel.destroy();
	this->dragonModel.destroy();

	this->terrain.destroy();
	this->camera.destroy();
}

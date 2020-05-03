#include "SandboxLayer.h"

#include <glTF/stb_image.h>

#include "Engine/Core/Scene/GLTFLoader.h"
#include "Engine/Core/Graphics/Renderer.h"
#include "Engine/Core/Display/Display.h"
#include "Engine/Core/Input/Input.h"
#include "Engine/Core/Audio/AudioSystem.h"
#include "Engine/Core/Scene/ObjectManager.h"

void SandboxLayer::onStart(ym::Renderer* renderer)
{
	ym::GLTFLoader::loadOnThread(YM_ASSETS_FILE_PATH + "Models/Tree/Tree.glb", &this->treeModel);
	ym::GLTFLoader::loadOnThread(YM_ASSETS_FILE_PATH + "Models/Cube/Cube.gltf", &this->cubeModel);
	ym::GLTFLoader::loadOnThread(YM_ASSETS_FILE_PATH + "Models/WaterBottle/WaterBottle.glb", &this->waterBottleModel);
	ym::GLTFLoader::loadOnThread(YM_ASSETS_FILE_PATH + "Models/Sponza/glTF/Sponza.gltf", &this->sponzaModel);

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
	this->cameraLockSound->setVolume(1.f);
	this->cameraUnlockSound = ym::AudioSystem::get()->createSound(YM_ASSETS_FILE_PATH + "/Audio/SoundEffects/ButtonOn.mp3");
	this->cameraUnlockSound->setVolume(1.f);
	
	this->ambientSound = ym::AudioSystem::get()->createStream(YM_ASSETS_FILE_PATH + "/Audio/Ambient/Rainforest.mp3");
	this->ambientSound->play();
	this->ambientSound->setVolume(0.2f);

	this->music = ym::AudioSystem::get()->createStream(YM_ASSETS_FILE_PATH + "/Audio/Music/DunnoJBPet.mp3");


	// Cube
	glm::mat4 transformCube(100.0f);
	transformCube[3][3] = 1.0f;
	transformCube = glm::translate(glm::mat4(1.0f), { 0.0f, -100.f, 0.f }) * transformCube;
	this->cubeObject = ym::ObjectManager::get()->createGameObject(transformCube, &this->cubeModel);

	glm::mat4 transformSponza(1.0f);
	transformSponza[3][3] = 1.0f;
	transformSponza = glm::translate(glm::mat4(1.0f), { 0.0f, 1.f, 0.f }) * transformSponza;
	this->sponzaObject = ym::ObjectManager::get()->createGameObject(transformSponza, &this->sponzaModel);
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
		this->music->setVolume(0.2f);
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
	static int32_t maxTrees = 100;
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
}

void SandboxLayer::onRender(ym::Renderer* renderer)
{
	renderer->begin();

	
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
	this->terrain.destroy();
	this->camera.destroy();
}

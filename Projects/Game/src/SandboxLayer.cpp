#include "SandboxLayer.h"

#include "Engine/Core/Scene/GLTFLoader.h"
#include "Engine/Core/Graphics/Renderer.h"
#include "Engine/Core/Display/Display.h"
#include "Engine/Core/Input/Input.h"

void SandboxLayer::onStart(ym::Renderer* renderer)
{
	ym::GLTFLoader::loadOnThread(YM_ASSETS_FILE_PATH + "Models/Tree/Tree.glb", &this->model);
	ym::GLTFLoader::loadOnThread(YM_ASSETS_FILE_PATH + "Models/Cube/Cube.gltf", &this->cubeModel);
	ym::GLTFLoader::loadOnThread(YM_ASSETS_FILE_PATH + "Models/WaterBottle/WaterBottle.glb", &this->waterBottleModel);
	//ym::GLTFLoader::loadOnThread(YM_ASSETS_FILE_PATH + "Models/Sponza/glTF/Sponza.gltf", &this->sponzaModel);

	float aspect = ym::Display::get()->getAspectRatio();
	this->camera.init(aspect, 45.f, { 0.f, 2, 0.f }, { 0.f, 2, 1.f }, 1.0f, 10.0f);

	renderer->setCamera(&this->camera);
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
		}
		else input->unlockMouse();
	}

	if (lock)
	{
		input->centerMouse();
		this->camera.update(dt);
	}

	// Quit if ESCAPE is pressed.
	if (input->isKeyPressed(ym::Key::ESCAPE))
		this->terminate();
}

void SandboxLayer::onRender(ym::Renderer* renderer)
{
	renderer->begin();

	static int32_t max = 20;
	static float r = max;
	ym::Input* input = ym::Input::get();
	ym::KeyState keyState = input->getKeyState(ym::Key::E);
	if (keyState == ym::KeyState::FIRST_RELEASED)
		r = ++max;
	std::vector<glm::mat4> transforms;
	for (int32_t i = 0; i < max; i++)
	{
		
		float angle = (float)i / (float)max * 2*glm::pi<float>();
		float x = glm::cos(angle)*r;
		float z = glm::sin(angle)*r;
		auto transform = glm::mat4(1.0f);
		transform = glm::translate(glm::mat4(1.0f), { x, 0.f, z}) * transform;
		transforms.push_back(transform);
	}
	renderer->drawModel(&this->model, transforms);

	static bool spawn = false;
	keyState = input->getKeyState(ym::Key::F);
	if (keyState == ym::KeyState::FIRST_RELEASED)
		spawn ^= 1;
	if (spawn)
	{
		auto transform = glm::mat4(10.0f);
		transform[3][3] = 1.0f;
		transform = glm::translate(glm::mat4(1.0f), { 0.0f, 2.f, 0.f }) * transform;
		renderer->drawModel(&this->waterBottleModel, transform);
	}

	glm::mat4 transform(100.0f);
	transform[3][3] = 1.0f;
	transform = glm::translate(glm::mat4(1.0f), { 0.0f, -100.f, 0.f }) * transform;
	renderer->drawModel(&this->cubeModel, transform);
	
	//transform = glm::mat4(1.0f);
	//transform = glm::translate(glm::mat4(1.0f), { 0.0f, 1.f, 0.f }) * transform;
	//renderer->drawModel(&this->sponzaModel, transform);

	renderer->end();
}

void SandboxLayer::onRenderImGui()
{
}

void SandboxLayer::onQuit()
{
	ym::Input::get()->unlockMouse();
	this->model.destroy();
	this->cubeModel.destroy();
	this->waterBottleModel.destroy();
	//this->sponzaModel.destroy();
	this->camera.destroy();
}

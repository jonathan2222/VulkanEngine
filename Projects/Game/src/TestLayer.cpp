#include "TestLayer.h"

#include "Engine/Core/Scene/GLTFLoader.h"
#include "Engine/Core/Graphics/Renderer.h"
#include "Engine/Core/Display/Display.h"
#include "Engine/Core/Scene/ObjectManager.h"

void TestLayer::onStart(ym::Renderer* renderer)
{
	ym::GLTFLoader::loadOnThread(YM_ASSETS_FILE_PATH + "Models/Cube/Cube.gltf", &this->cubeModel);

	float aspect = ym::Display::get()->getAspectRatio();
	this->camera.init(aspect, 45.f, { 0.f, 2, 0.f }, { 0.f, 2, 1.f }, 1.0f, 10.0f);
	renderer->setActiveCamera(&this->camera);

	glm::mat4 transformCube(0.5f);
	transformCube[3][3] = 1.0f;
	transformCube = glm::translate(glm::mat4(1.0f), { 0.0f, 1.f, 3.f }) * transformCube;
	this->cubeObject = ym::ObjectManager::get()->createGameObject(transformCube, &this->cubeModel);

	this->environmentMap = renderer->getDefaultEnvironmentMap();
}

void TestLayer::onUpdate(float dt)
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
		else
		{
			input->unlockMouse();
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
}

void TestLayer::onRender(ym::Renderer* renderer)
{
	renderer->begin();

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

	glm::mat4 transform(1.f);
	renderer->drawSkybox(this->environmentMap);
	renderer->drawAllModels(ym::ObjectManager::get());

	renderer->end();
}

void TestLayer::onRenderImGui()
{
}

void TestLayer::onQuit()
{
	ym::Input::get()->unlockMouse();
	this->cubeModel.destroy();
	this->camera.destroy();
}

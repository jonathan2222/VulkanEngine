#include "SandboxLayer.h"

#include "Engine/Core/Scene/GLTFLoader.h"
#include "Engine/Core/Graphics/Renderer.h"
#include "Engine/Core/Display/Display.h"
#include "Engine/Core/Input/Input.h"

void SandboxLayer::onStart(ym::Renderer* renderer)
{
	ym::GLTFLoader::load(YM_ASSETS_FILE_PATH + "Models/Tree/Tree.glb", &this->model);

	float aspect = ym::Display::get()->getAspectRatio();
	this->camera.init(aspect, 45.f, { 0.f, 0, 0.f }, { 0.f, 0, 1.f }, 10.0f, 10.0f);

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
}

void SandboxLayer::onRender(ym::Renderer* renderer)
{
	renderer->begin();
	auto transform = glm::mat4(1.0f);
	transform = glm::translate(transform, {0.f, 0.f, 10.f});
	renderer->drawModel(&this->model, transform);
	renderer->end();
}

void SandboxLayer::onRenderImGui()
{
}

void SandboxLayer::onQuit()
{
	ym::Input::get()->unlockMouse();
	this->model.destroy();
	this->camera.destroy();
}

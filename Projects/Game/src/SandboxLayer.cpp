#include "SandboxLayer.h"

#include "Engine/Core/Scene/GLTFLoader.h"
#include "Engine/Core/Graphics/Renderer.h"

void SandboxLayer::onStart(ym::Renderer* renderer)
{
	ym::GLTFLoader::load(YM_ASSETS_FILE_PATH + "Models/Cube/Cube.gltf", &this->model);
}

void SandboxLayer::onUpdate(float dt)
{
}

void SandboxLayer::onRender(ym::Renderer* renderer)
{
	renderer->begin();
	auto transform = glm::mat4(1.0f);
	renderer->drawModel(&this->model, transform);
	renderer->end();
}

void SandboxLayer::onRenderImGui()
{
}

void SandboxLayer::onQuit()
{
	this->model.destroy();
}

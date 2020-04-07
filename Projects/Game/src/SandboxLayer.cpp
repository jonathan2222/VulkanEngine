#include "SandboxLayer.h"

#include "Engine/Core/Scene/GLTFLoader.h"

void SandboxLayer::onStart()
{
	ym::GLTFLoader::load(YM_ASSETS_FILE_PATH + "Models/Cube/Cube.gltf", &this->model);
}

void SandboxLayer::onUpdate(float dt)
{
}

void SandboxLayer::onRender(ym::Renderer* renderer)
{
}

void SandboxLayer::onRenderImGui()
{
}

void SandboxLayer::onQuit()
{
	this->model.destroy();
}

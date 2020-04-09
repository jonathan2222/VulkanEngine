#pragma once

#include "Engine/Core/Application/Layer.h"
#include "Engine/Core/Scene/Model/Model.h"
#include "Engine/Core/Camera.h"

class SandboxLayer : public ym::Layer
{
public:

	void onStart(ym::Renderer* renderer) override;
	void onUpdate(float dt) override;
	void onRender(ym::Renderer* renderer) override;
	void onRenderImGui() override;
	void onQuit() override;

private:
	ym::Model model;
	ym::Camera camera;
};
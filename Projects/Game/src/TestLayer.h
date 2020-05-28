#pragma once

#include "Engine/Core/Application/Layer.h"
#include "Engine/Core/Scene/Model/Model.h"
#include "Engine/Core/Camera.h"

#include "Engine/Core/Scene/GameObject.h"

class TestLayer : public ym::Layer
{
public:

	void onStart(ym::Renderer* renderer) override;
	void onUpdate(float dt) override;
	void onRender(ym::Renderer* renderer) override;
	void onRenderImGui() override;
	void onQuit() override;

private:
	ym::Model cubeModel;
	ym::Camera camera;

	ym::Texture* environmentMap;

	ym::GameObject* cubeObject;
};
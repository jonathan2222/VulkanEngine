#pragma once

#include "Engine/Core/Application/Layer.h"
#include "Engine/Core/Scene/Model/Model.h"
#include "Engine/Core/Scene/Model/CubeMap.h"
#include "Engine/Core/Scene/Terrain/Terrain.h"
#include "Engine/Core/Camera.h"

#include "Engine/Core/Audio/Sound.h"

#include "Engine/Core/Scene/GameObject.h"

class SandboxLayer : public ym::Layer
{
public:

	void onStart(ym::Renderer* renderer) override;
	void onUpdate(float dt) override;
	void onRender(ym::Renderer* renderer) override;
	void onRenderImGui() override;
	void onQuit() override;

private:
	ym::Model treeModel;
	ym::Model cubeModel;
	ym::Model waterBottleModel;
	ym::Model sponzaModel;
	ym::Model chestModel;
	ym::Model woodenCrateModel;
	ym::Model fortModel;
	ym::Model terrain2Model;
	ym::Model metalBallsModel;
	ym::Terrain terrain;
	ym::Camera camera;

	ym::Texture* environmentMap;

	ym::Sound* cameraLockSound;
	ym::Sound* cameraUnlockSound;
	ym::Sound* ambientSound;
	ym::Sound* music;
	ym::Sound* pokerChips;

	std::vector<ym::GameObject*> treesObjects;
	ym::GameObject* watterBottleObject;
	ym::GameObject* cubeObject;
	ym::GameObject* sponzaObject;
	ym::GameObject* chestObject;
	ym::GameObject* woodenCrateObject;
	ym::GameObject* fortObject;
};
#pragma once

#include "Engine/Core/Application/Layer.h"

class SandboxLayer : public ym::Layer
{
public:

	void onStart() override;
	void onUpdate(float dt) override;
	void onRender() override;
	void onRenderImGui() override;
	void onQuit() override;

private:

};
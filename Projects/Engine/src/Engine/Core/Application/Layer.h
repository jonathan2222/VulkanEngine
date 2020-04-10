#pragma once

#include "Engine/Core/Input/Input.h"
#include "Engine/Core/Logger.h"
#include "Engine/Core/Input/Config.h"

#include "LayerManager.h"

namespace ym
{
	class Renderer;
	class Layer
	{
	public:
		Layer();
		virtual ~Layer();

		virtual void onStart(ym::Renderer* renderer) = 0;
		virtual void onUpdate(float dt) = 0;
		virtual void onRender(Renderer* renderer) = 0;
		virtual void onRenderImGui() = 0;
		virtual void onQuit() = 0;

		void terminate();

		LayerManager* getManager();

		friend class LayerManager;
	};
}
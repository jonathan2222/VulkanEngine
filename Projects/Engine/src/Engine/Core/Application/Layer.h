#pragma once

#include "Engine/Core/Input/Input.h"
#include "Engine/Core/Logger.h"
#include "Engine/Core/Input/Config.h"

#include "LayerManager.h"

namespace ym
{
	class Layer
	{
	public:
		Layer();
		virtual ~Layer();

		virtual void onStart() = 0;
		virtual void onUpdate(float dt) = 0;
		virtual void onRender() = 0;
		virtual void onRenderImGui() = 0;
		virtual void onQuit() = 0;

		LayerManager* getManager();

		friend class LayerManager;
	};
}
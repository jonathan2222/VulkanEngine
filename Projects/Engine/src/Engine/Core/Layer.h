#pragma once

#include "Engine/Core/Input/Input.h"
#include "Engine/Core/Logger.h"
#include "Engine/Core/Input/Config.h"

#include "API.h"
#include "Input/Input.h"
#include "Display.h"
#include "Graphics/Renderer.h"

#include "LayerManager.h"

#include "ImGuiImpl.h"

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

		API* getAPI();
		Input* getInput();
		Display* getDisplay();
		Renderer* getRenderer();
		LayerManager* getManager();

		friend class LayerManager;

	private:
		void init(API* api, Input* input, Display* display, Renderer* renderer);

		// Shared between layers.
		API* m_api;
		Input* m_input;
		Display* m_display;
		Renderer* m_renderer;
	};
}
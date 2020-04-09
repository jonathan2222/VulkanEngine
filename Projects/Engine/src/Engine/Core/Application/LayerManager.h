#pragma once

#include <vector>

#include "Engine/Core/Vulkan/CommandPools.h"

namespace ym
{
	class API;
	class App;
	class Layer;
	class Renderer;
	struct CommandPools;
	class LayerManager
	{
	public:
		LayerManager() = default;
		static LayerManager* get();

		void setApp(App* app);

		// Push a new layer as the new active layer.
		void push(Layer* layer);
		// Pop the activa layer, and set the new activa layer as the one under it.
		void pop();

		// Initiate all layers
		void onStart(Renderer* renderer);
		// Update the active layer
		void onUpdate(float dt);
		// Render the active layer
		void onRender(Renderer* renderer);
		// Render ImGui on the active layer
		void onRenderImGui();

		// Quit all layers
		void onQuit();

		CommandPools* getCommandPools();

	private:
		virtual ~LayerManager();

		std::vector<Layer*> layers;
		App* appPtr;
	};
}
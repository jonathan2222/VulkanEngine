#pragma once

#include <vector>

namespace ym
{
	class API;
	class App;
	class Layer;
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
		void onStart();
		// Update the active layer
		void onUpdate(float dt);
		// Render the active layer
		void onRender();
		// Render ImGui on the active layer
		void onRenderImGui();

		// Quit all layers
		void onQuit();

	private:
		virtual ~LayerManager();

		std::vector<Layer*> layers;
		App* appPtr;
	};
}
#pragma once
 
#include "../API.h"

#include "Layer.h"
#include <vector>
#include "../Graphics/Renderer.h"

int main(int argc, char* argv[]);

namespace ym
{
	class App
	{
	public:
		App();
		virtual ~App();

		virtual void start() = 0;

		void run();

		LayerManager* getLayerManager();

		friend class LayerManager;

	protected:
		ym::LayerManager* layerManager;
		Renderer renderer;
	};

	// The user should define this!
	App* createApplication();
};
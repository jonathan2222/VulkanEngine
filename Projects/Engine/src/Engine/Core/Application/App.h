#pragma once
 
#include "../API.h"

#include "Layer.h"
#include <vector>

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
	};

	// The user should define this!
	App* createApplication();
};
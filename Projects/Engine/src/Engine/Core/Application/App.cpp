#include "stdafx.h"
#include "App.h"

#include "../Display/Display.h"
#include "../Input/Input.h"
#include "LayerManager.h"

#include "../Graphics/VulkanInstance.h"

#include "Utils/Timer.h"

ym::App::App()
{
	YM_PROFILER_BEGIN_SESSION("Initialization", "CPU_Profiler_Initialization.json");

	YM_PROFILER_FUNCTION();

	YM_LOG_INIT();
	ym::Config::get()->init(YM_CONFIG_FILE_PATH);
	ym::Config::get()->print();
	
	DisplayDesc displayDesc;
	displayDesc.init();

	API* api = API::get();
	api->init();

	// Fetch the first instance of display.
	Display* display = Display::get();
	display->setDescription(displayDesc);
	display->init();

	// Initialize the input manager.
	Input* input = Input::get();
	input->init();

	// Initialize the vulkan instance.
	VulkanInstance* instance = VulkanInstance::get();
	instance->init({}, {});

	// Create the layer manager.
	this->layerManager = LayerManager::get();
	this->layerManager->setApp(this);
}

ym::App::~App()
{
	VulkanInstance::get()->destroy();
	Display::get()->destroy();
	API::get()->destroy();

	YM_PROFILER_END_SESSION();
}

void ym::App::run()
{
	bool activateImGUI = Config::get()->fetch<bool>("Debuglayer/active");

	start();

	// Initiate layers
	this->layerManager->onStart();

	YM_PROFILER_END_SESSION();

#ifdef YAMI_DEBUG
	unsigned long long renderingProfilingFrameCounter = 0;
	const unsigned long long renderingProfilingFrameCountMax = 10;
#endif

	Display* display = Display::get();

	ym::Timer timer;
	float dt = 0.16f;
	float debugTimer = 1.0f;
	while (!display->shouldClose())
	{
		// Begin rendering profiling.
#ifdef YAMI_DEBUG
		if (ym::Input::get()->isKeyPressed(ym::Key::P))
		{
			if(ym::Instrumentation::g_runRenderingProfiling == false)
				renderingProfilingFrameCounter = 0;
			YM_PROFILER_RENDERING_BEGIN_SESSION("Rendering", "CPU_Profiler_Rendering.json");
		}
#endif

		// Frame
		YM_PROFILER_RENDERING_SCOPE("Frame");

		timer.start();
		display->pollEvents();

		// Update
		{
			YM_PROFILER_RENDERING_SCOPE("Update");
			// Update the active layer
			this->layerManager->onUpdate(dt);
		}

		// Render
		{
			YM_PROFILER_RENDERING_SCOPE("Render");
			// Begin frame
			//m_renderer->beginScene(0.0f, 0.0f, 0.0f, 1.0f);

			// Render the active layer
			this->layerManager->onRender();

			// Render debug information on the active layer with ImGUI

			// End frame
			//m_renderer->endScene();
		}

		dt = timer.stop();
		debugTimer += dt;
		if (debugTimer >= 1.0f)
		{
			debugTimer = 0.0f;
			YM_LOG_INFO("dt: {0:.2f} ms (FPS: {1:.2f})", dt * 1000.f, 1.0f / dt);
		}

#ifdef YAMI_DEBUG
		renderingProfilingFrameCounter++;
		if (renderingProfilingFrameCounter >= renderingProfilingFrameCountMax)
		{
			YM_PROFILER_RENDERING_END_SESSION();
		}
#endif
	}


	YM_PROFILER_BEGIN_SESSION("Quit", "CPU_Profiler_Quit.json");

	// Shutdown layers
	this->layerManager->onQuit();
}

ym::LayerManager* ym::App::getLayerManager()
{
	return this->layerManager;
}
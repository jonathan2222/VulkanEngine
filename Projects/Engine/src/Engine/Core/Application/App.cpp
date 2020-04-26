#include "stdafx.h"
#include "App.h"

#include "../Display/Display.h"
#include "../Input/Input.h"
#include "LayerManager.h"
#include "../Threading/ThreadManager.h"
#include "Engine/Core/Audio/AudioSystem.h"

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

	AudioSystem::get()->init();

	API* api = API::get();
	api->init();

	// Fetch the first instance of display.
	Display* display = Display::get();
	display->setDescription(displayDesc);
	display->init();

	// Initialize the input manager.
	Input* input = Input::get();
	input->init();

	this->vulkanInstance = VulkanInstance::get();
	this->vulkanInstance->init();

	this->commandPools.init();

	// Create the layer manager.
	this->layerManager = LayerManager::get();
	this->layerManager->setApp(this);

	// Initialize the vulkan renderer.
	this->renderer.init();

	// Allocate threads for each renderer and one for the GLTFLoader.
	ThreadManager::init((uint32_t)Renderer::ERendererType::RENDER_TYPE_SIZE + 1);
}

ym::App::~App()
{
	ThreadManager::destroy();

	this->renderer.destroy();
	this->commandPools.destroy();
	this->vulkanInstance->destroy();
	Display::get()->destroy();
	API::get()->destroy();
	AudioSystem::get()->destroy();

	YM_PROFILER_END_SESSION();
}

void ym::App::run()
{
	bool activateImGUI = Config::get()->fetch<bool>("Debuglayer/active");

	// Initiate layers
	this->layerManager->onStart(&this->renderer);

	YM_PROFILER_END_SESSION();

#ifdef YM_DEBUG
	unsigned long long renderingProfilingFrameCounter = 0;
	const unsigned long long renderingProfilingFrameCountMax = 10;
#endif

	Display* display = Display::get();

	ym::Timer timer;
	float dt = 0.16f;
	float debugTimer = 1.0f;
	while (!(display->shouldClose() || this->shouldQuit))
	{
		// Begin rendering profiling.
#ifdef YM_DEBUG
		/*if (ym::Input::get()->isKeyPressed(ym::Key::P))
		{
			if(ym::Instrumentation::g_runRenderingProfiling == false)
				renderingProfilingFrameCounter = 0;
			YM_PROFILER_RENDERING_BEGIN_SESSION("Rendering", "CPU_Profiler_Rendering.json");
		}*/
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
			AudioSystem::get()->update();
		}
		// Render
		{
			YM_PROFILER_RENDERING_SCOPE("Render");
			// Begin frame
			//m_renderer->beginScene(0.0f, 0.0f, 0.0f, 1.0f);

			// Render the active layer
			this->layerManager->onRender(&this->renderer);

			// Render debug information on the active layer with ImGUI

			// End frame
			//m_renderer->endScene();
		}

		Input::get()->update();

		dt = timer.stop();
		debugTimer += dt;
		if (debugTimer >= 1.0f)
		{
			debugTimer = 0.0f;
			YM_LOG_INFO("dt: {0:.2f} ms (FPS: {1:.2f})", dt * 1000.f, 1.0f / dt);
		}

#ifdef YM_DEBUG
		renderingProfilingFrameCounter++;
		if (renderingProfilingFrameCounter >= renderingProfilingFrameCountMax)
		{
			YM_PROFILER_RENDERING_END_SESSION();
		}
#endif
	}


	YM_PROFILER_BEGIN_SESSION("Quit", "CPU_Profiler_Quit.json");

	// Shutdown layers
	this->renderer.preDestroy();
	this->layerManager->onQuit();
}

void ym::App::terminate()
{
	this->shouldQuit = true;
}

ym::LayerManager* ym::App::getLayerManager()
{
	return this->layerManager;
}

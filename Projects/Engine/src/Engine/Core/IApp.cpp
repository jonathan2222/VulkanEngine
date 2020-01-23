#include "stdafx.h"
#include "IApp.h"

#include "API.h"
#include "ImGuiImpl.h"
#include "Display.h"
#include "Input/Input.h"
#include "Graphics/Renderer.h"
#include "LayerManager.h"

#include "Utils/Timer.h"

ym::IApp::IApp(DisplayDesc& displayDescriptor) : m_display(nullptr), m_input(nullptr), m_imGuiImpl(nullptr)
{
	YM_PROFILER_BEGIN_SESSION("Initialization", "CPU_Profiler_Initialization.json");

	YM_PROFILER_FUNCTION();

	YM_LOG_INIT();
	ym::Config::get()->init(YM_CONFIG_FILE_PATH);
	displayDescriptor.init();
	std::string apiType = Config::get()->fetch<std::string>("API/type");
	YM_LOG_INFO("Using API: {0}", apiType.c_str());
	
	KeyConverter::init();

	m_api = API::get();
	m_api->preDisplayInit(displayDescriptor);
	m_display = Display::create(displayDescriptor);
	m_api->postDisplayInit();

	m_renderer = Renderer::get();
	m_renderer->init(m_display->getDescription());

	API::VideoCardInfo& gpuInfo = m_api->getVideoCardInfo();
	YM_LOG_INFO("GPU: {0}", gpuInfo.name.c_str());

	m_input = ym::Input::create();
	m_input->init();

	m_layerManager = LayerManager::get();
	m_layerManager->setApp(this);

	bool activateImGUI = Config::get()->fetch<bool>("ImGUI/active");
	if (activateImGUI)
	{
		m_imGuiImpl = ImGuiImpl::create();
		m_imGuiImpl->setUp();
		m_display->setImGuiImpl(m_imGuiImpl);
	}
}

ym::IApp::~IApp()
{
	bool activateImGUI = Config::get()->fetch<bool>("ImGUI/active");
	if (activateImGUI)
	{
		m_imGuiImpl->cleanUp();
		delete m_imGuiImpl;
	}
	m_renderer->destroy();

	delete m_display;
	m_api->destroy();
	delete m_input;

	YM_PROFILER_END_SESSION();
}

void ym::IApp::run()
{
	bool activateImGUI = Config::get()->fetch<bool>("ImGUI/active");

	start();

	// Initiate layers
	m_layerManager->onStart();

	YM_PROFILER_END_SESSION();

#ifdef YAMI_DEBUG
	unsigned long long renderingProfilingFrameCounter = 0;
	const unsigned long long renderingProfilingFrameCountMax = 10;
#endif

	ym::Timer timer;
	float dt = 0.16f;
	float debugTimer = 1.0f;
	while (!m_display->shouldClose())
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
		m_display->pollEvents();

		// Update
		{
			YM_PROFILER_RENDERING_SCOPE("Update");
			// Update the active layer
			m_layerManager->onUpdate(dt);
		}

		// Render
		{
			YM_PROFILER_RENDERING_SCOPE("Render");
			// Begin frame
			m_renderer->beginScene(0.0f, 0.0f, 0.0f, 1.0f);

			// Render the active layer
			m_layerManager->onRender();

			// Render debug information on the active layer with ImGUI
			if (activateImGUI)
			{
				YM_PROFILER_RENDERING_SCOPE("ImGUI Rendering");

				m_imGuiImpl->startFrame();
				m_layerManager->onRenderImGui();
				m_imGuiImpl->endFrame();
			}

			// End frame
			m_renderer->endScene();
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
	m_layerManager->onQuit();
}

ym::LayerManager* ym::IApp::getLayerManager()
{
	return m_layerManager;
}

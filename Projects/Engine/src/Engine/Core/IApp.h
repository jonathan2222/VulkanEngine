#pragma once

#include "Layer.h"
#include <vector>

namespace ym
{
	class ImGuiImpl;
	class LayerManager;
	class Renderer;
	struct DisplayDesc;
	class Display;
	class Input;
	class API;
	class IApp
	{
	public:
		IApp(DisplayDesc& displayDescriptor);
		virtual ~IApp();

		virtual void processArguments(int argc, char* argv[]) = 0;

		virtual void start() = 0;

		void run();

		LayerManager* getLayerManager();

		friend class LayerManager;
	protected:
		ym::API* m_api;
		ym::Input* m_input;
		ym::Display* m_display;
		ym::Renderer* m_renderer;
		ym::LayerManager* m_layerManager;
		ym::ImGuiImpl* m_imGuiImpl;
	};
};
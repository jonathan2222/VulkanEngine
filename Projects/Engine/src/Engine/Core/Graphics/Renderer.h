#pragma once

#include "VulkanInstance.h"
#include "SwapChain.h"

namespace ym
{
	class Renderer
	{
	public:
		Renderer();
		virtual ~Renderer();

		void init();
		void destroy();

	private:
		VulkanInstance* instance;
		SwapChain swapChain;
	};
}
#pragma once

#include "Engine/Core/Vulkan/VulkanInstance.h"
#include "Engine/Core/Vulkan/SwapChain.h"

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
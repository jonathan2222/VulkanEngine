#pragma once

#include "Engine/Core/Vulkan/VulkanInstance.h"
#include "Engine/Core/Vulkan/SwapChain.h"

namespace ym
{
	class ModelRenderer
	{
	public:
		ModelRenderer();
		virtual ~ModelRenderer();

		void init();
		void destroy();

	private:
		VulkanInstance* instance;
		SwapChain swapChain;
	};
}
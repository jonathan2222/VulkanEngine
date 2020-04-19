#pragma once

#include "Engine/Core/Vulkan/CommandPool.h"
#include "Engine/Core/Vulkan/SwapChain.h"

namespace ym
{
	struct ThreadData
	{
		uint32_t threadIndex;
		CommandPool graphicsPool;
		CommandPool computePool;
		CommandPool transferPool;
		std::vector<CommandBuffer*> secondaryCommandBuffersGraphics;
		std::vector<CommandBuffer*> secondaryCommandBuffersCompute;
		std::vector<CommandBuffer*> secondaryCommandBuffersTransfer;

		void init(uint32_t threadIndex, SwapChain* swapChain);
		void destory();
	};
}
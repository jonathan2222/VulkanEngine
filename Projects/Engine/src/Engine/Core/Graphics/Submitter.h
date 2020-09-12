#pragma once

#include <vector>
#include "Engine/Core/Vulkan/CommandBuffer.h"

namespace ym
{
	class Submitter
	{
	public:

		void init();
		void destroy();

		void begin();
		void submitCompute(const std::vector<VkCommandBuffer>& buffers);
		void submitGraphics(const std::vector<VkCommandBuffer>& buffers);
		void end();

	private:

	};
}
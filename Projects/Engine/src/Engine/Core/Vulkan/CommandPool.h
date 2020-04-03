#pragma once

#include "stdafx.h"

namespace ym
{
	struct VulkanQueue;
	class CommandBuffer;

	class CommandPool
	{
	public:
		enum class Queue {
			GRAPHICS = VK_QUEUE_GRAPHICS_BIT,
			COMPUTE = VK_QUEUE_COMPUTE_BIT,
			TRANSFER = VK_QUEUE_TRANSFER_BIT
		};
	public:
		CommandPool();
		~CommandPool();

		void init(Queue queueFamily, VkCommandPoolCreateFlags flags);
		void destroy();

		Queue getQueueFamily() const { return this->queueFamily; }
		VulkanQueue getQueue() const;
		VkCommandPool getCommandPool() const { return this->pool; };

		CommandBuffer* beginSingleTimeCommand();
		void endSingleTimeCommand(CommandBuffer* buffer);

		CommandBuffer* createCommandBuffer(VkCommandBufferLevel level);
		std::vector<CommandBuffer*> createCommandBuffers(uint32_t count, VkCommandBufferLevel level);
		void removeCommandBuffer(CommandBuffer* buffer);

	private:
		void createCommandPool(VkCommandPoolCreateFlags flags);

		VkCommandPool pool;
		Queue queueFamily;
		std::vector<CommandBuffer*> buffers;
	};
}
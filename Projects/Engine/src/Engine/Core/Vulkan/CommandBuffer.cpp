#include "stdafx.h"
#include "CommandBuffer.h"
#include "CommandPool.h"
#include "VulkanInstance.h"
#include "Buffers/Buffer.h"
#include "Pipeline/RenderPass.h"
#include "Pipeline/Pipeline.h"
#include "Pipeline/PushConstants.h"

namespace ym
{
	CommandBuffer::CommandBuffer() :
		pool(VK_NULL_HANDLE), buffer(VK_NULL_HANDLE)
	{
	}

	CommandBuffer::~CommandBuffer()
	{
	}

	void CommandBuffer::init(CommandPool* pool)
	{
		this->pool = pool;
	}

	void CommandBuffer::destroy()
	{
		// Commands buffers are cleared automatically by the command pool
	}

	void CommandBuffer::setCommandBuffer(VkCommandBuffer buffer)
	{
		this->buffer = buffer;
	}

	void CommandBuffer::createCommandBuffer(VkCommandBufferLevel level)
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = this->pool->getCommandPool();
		allocInfo.level = level;
		allocInfo.commandBufferCount = 1;
		allocInfo.pNext = nullptr;

		VULKAN_CHECK(vkAllocateCommandBuffers(VulkanInstance::get()->getLogicalDevice(), &allocInfo, &this->buffer), "Failed to allocate command buffer!");
	}

	void CommandBuffer::begin(VkCommandBufferUsageFlags flags, VkCommandBufferInheritanceInfo* instanceInfo)
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = flags; // Optional
		beginInfo.pInheritanceInfo = instanceInfo; // Optional

		VULKAN_CHECK(vkBeginCommandBuffer(this->buffer, &beginInfo), "Failed to begin recording command buffer!");
	}

	void CommandBuffer::cmdBeginRenderPass(RenderPass* renderPass, VkFramebuffer framebuffer, VkExtent2D extent, const std::vector<VkClearValue>& clearValues, VkSubpassContents subpassContents)
	{
		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass->getRenderPass();
		renderPassInfo.framebuffer = framebuffer;
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = extent;
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();
		renderPassInfo.pNext = nullptr;

		vkCmdBeginRenderPass(this->buffer, &renderPassInfo, subpassContents);
	}

	void CommandBuffer::cmdBindPipeline(Pipeline* pipeline)
	{
		vkCmdBindPipeline(this->buffer, (VkPipelineBindPoint)pipeline->getType(), pipeline->getPipeline());
	}

	void CommandBuffer::cmdBindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* buffers, const VkDeviceSize* offsets)
	{
		vkCmdBindVertexBuffers(this->buffer, firstBinding, bindingCount, buffers, offsets);
	}

	void CommandBuffer::cmdBindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
	{
		vkCmdBindIndexBuffer(this->buffer, buffer, offset, indexType);
	}

	void CommandBuffer::cmdPushConstants(Pipeline* pipeline, const PushConstants* pushConstant)
	{
		for (auto& range : pushConstant->getRangeMap())
			vkCmdPushConstants(this->buffer, pipeline->getPipelineLayout(), range.first, range.second.offset, range.second.size, (void*)((char*)pushConstant->getData() + range.second.offset));
	}

	void CommandBuffer::cmdPushConstants(Pipeline * pipeline, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* data)
	{
		vkCmdPushConstants(this->buffer, pipeline->getPipelineLayout(), stageFlags, offset, size, data);
	}

	void CommandBuffer::cmdBindDescriptorSets(Pipeline * pipeline, uint32_t firstSet, const std::vector<VkDescriptorSet> & sets, const std::vector<uint32_t> & offsets)
	{
		vkCmdBindDescriptorSets(this->buffer, (VkPipelineBindPoint)pipeline->getType(),
			pipeline->getPipelineLayout(), 0, static_cast<uint32_t>(sets.size()), sets.data(), static_cast<uint32_t>(offsets.size()), offsets.data());
	}

	void CommandBuffer::cmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
	{
		vkCmdDraw(this->buffer, vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void CommandBuffer::cmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance)
	{
		vkCmdDrawIndexed(this->buffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void CommandBuffer::cmdDrawIndexedIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
	{
		vkCmdDrawIndexedIndirect(this->buffer, buffer, offset, drawCount, stride);
	}

	void CommandBuffer::cmdMemoryBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlag, std::vector<VkMemoryBarrier> barriers)
	{
		vkCmdPipelineBarrier(
			this->buffer,
			srcStageMask,
			dstStageMask,
			dependencyFlag,
			(uint32_t)barriers.size(), barriers.data(),
			0, nullptr,
			0, nullptr);
	}

	void CommandBuffer::cmdBufferMemoryBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlag, std::vector<VkBufferMemoryBarrier>barriers)
	{
		vkCmdPipelineBarrier(this->buffer,
			srcStageMask,
			dstStageMask,
			dependencyFlag,
			0, nullptr,
			(uint32_t)barriers.size(), barriers.data(),
			0, nullptr);
	}

	void CommandBuffer::cmdImageMemoryBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlag, std::vector<VkImageMemoryBarrier> barriers)
	{
		vkCmdPipelineBarrier(this->buffer,
			srcStageMask,
			dstStageMask,
			dependencyFlag,
			0, nullptr,
			0, nullptr,
			(uint32_t)barriers.size(), barriers.data());
	}

	void CommandBuffer::cmdDispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
	{
		vkCmdDispatch(this->buffer, groupCountX, groupCountY, groupCountZ);
	}

	void CommandBuffer::cmdExecuteCommands(uint32_t bufferCount, const VkCommandBuffer * secondaryBuffers)
	{
		vkCmdExecuteCommands(this->buffer, bufferCount, secondaryBuffers);
	}

	void CommandBuffer::cmdEndRenderPass()
	{
		vkCmdEndRenderPass(this->buffer);
	}

	void CommandBuffer::end()
	{
		VULKAN_CHECK(vkEndCommandBuffer(this->buffer), "Failed to record command buffer!")
	}

	void CommandBuffer::cmdCopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy * pRegions)
	{
		vkCmdCopyBuffer(this->buffer, srcBuffer, dstBuffer, regionCount, pRegions);
	}

	void CommandBuffer::cmdCopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy * pRegions)
	{
		vkCmdCopyBufferToImage(this->buffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
	}

	void CommandBuffer::cmdWriteTimestamp(VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query)
	{
		vkCmdWriteTimestamp(this->buffer, pipelineStage, queryPool, query);
	}

	void CommandBuffer::cmdResetQueryPool(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount)
	{
		vkCmdResetQueryPool(this->buffer, queryPool, firstQuery, queryCount);
	}

	void CommandBuffer::cmdBeginQuery(VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags)
	{
		vkCmdBeginQuery(this->buffer, queryPool, query, flags);
	}

	void CommandBuffer::cmdEndQuery(VkQueryPool queryPool, uint32_t query)
	{
		vkCmdEndQuery(this->buffer, queryPool, query);
	}

	void CommandBuffer::acquireBuffer(Buffer * buffer, VkAccessFlags dstAccessMask, uint32_t srcQueue, uint32_t dstQueue, VkPipelineStageFlagBits srcStage, VkPipelineStageFlagBits dstStage)
	{
		VkBufferMemoryBarrier bufferBarrier = {};
		bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		bufferBarrier.buffer = buffer->getBuffer();
		bufferBarrier.size = buffer->getSize();
		bufferBarrier.srcAccessMask = 0;
		bufferBarrier.dstAccessMask = dstAccessMask;
		bufferBarrier.srcQueueFamilyIndex = srcQueue;
		bufferBarrier.dstQueueFamilyIndex = dstQueue;

		cmdBufferMemoryBarrier(srcStage, dstStage, 0, { bufferBarrier });
	}

	void CommandBuffer::releaseBuffer(Buffer * buffer, VkAccessFlags srcAccessMask, uint32_t srcQueue, uint32_t dstQueue, VkPipelineStageFlagBits srcStage, VkPipelineStageFlagBits dstStage)
	{
		VkBufferMemoryBarrier bufferBarrier = {};
		bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		bufferBarrier.buffer = buffer->getBuffer();
		bufferBarrier.size = buffer->getSize();
		bufferBarrier.srcAccessMask = srcAccessMask;
		bufferBarrier.dstAccessMask = 0;
		bufferBarrier.srcQueueFamilyIndex = srcQueue;
		bufferBarrier.dstQueueFamilyIndex = dstQueue;

		cmdBufferMemoryBarrier(srcStage, dstStage, 0, { bufferBarrier });
	}
}

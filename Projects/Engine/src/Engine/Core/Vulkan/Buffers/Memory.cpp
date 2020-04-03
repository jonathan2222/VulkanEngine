#include "stdafx.h"

#include "Memory.h"
#include "../VulkanInstance.h"
#include "Buffer.h"
#include "../VulkanHelperfunctions.h"
#include "../Texture.h"

namespace ym
{
	Memory::Memory() : memory(VK_NULL_HANDLE), currentOffset(0)
	{
	}

	Memory::~Memory()
	{
	}

	void Memory::init(VkMemoryPropertyFlags memProp)
	{
		YM_ASSERT(this->currentOffset != 0, "No buffers/images bound before allocation of memory!");

		uint32_t typeFilter = 0;
		for (auto buffer : this->bufferOffsets)
			typeFilter |= buffer.first->getMemReq().memoryTypeBits;
		for (auto texture : this->textureOffsets)
			typeFilter |= texture.first->getMemReq().memoryTypeBits;

		uint32_t memoryTypeIndex = findMemoryType(VulkanInstance::get()->getPhysicalDevice(), typeFilter, memProp);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = this->currentOffset;
		allocInfo.memoryTypeIndex = memoryTypeIndex;

		VULKAN_CHECK(vkAllocateMemory(VulkanInstance::get()->getLogicalDevice(), &allocInfo, nullptr, &this->memory), "Failed to allocate memory!");

		for (auto buffer : this->bufferOffsets)
			VULKAN_CHECK(vkBindBufferMemory(VulkanInstance::get()->getLogicalDevice(), buffer.first->getBuffer(), this->memory, buffer.second), "Failed to bind buffer memory!");

		for (auto texture : this->textureOffsets)
			VULKAN_CHECK(vkBindImageMemory(VulkanInstance::get()->getLogicalDevice(), texture.first->getVkImage(), this->memory, texture.second), "Failed to bind image memory!");
	}

	void Memory::destroy()
	{
		vkFreeMemory(VulkanInstance::get()->getLogicalDevice(), this->memory, nullptr);
	}

	void Memory::bindBuffer(Buffer* buffer)
	{
		this->bufferOffsets[buffer] = this->currentOffset;
		this->currentOffset += static_cast<Offset>(buffer->getMemReq().size);
	}

	void Memory::bindTexture(Texture* texture)
	{
		this->textureOffsets[texture] = this->currentOffset;
		this->currentOffset += static_cast<Offset>(texture->getMemReq().size);
	}

	void Memory::directTransfer(Buffer* buffer, const void* data, uint64_t size, Offset bufferOffset)
	{
		Offset offset = this->bufferOffsets[buffer];

		void* ptrGpu;
		VULKAN_CHECK(vkMapMemory(VulkanInstance::get()->getLogicalDevice(), this->memory, offset + bufferOffset, VK_WHOLE_SIZE, 0, &ptrGpu), "Failed to map memory for buffer!");
		memcpy(ptrGpu, data, size);
		vkUnmapMemory(VulkanInstance::get()->getLogicalDevice(), this->memory);
	}

	VkDeviceMemory Memory::getMemory()
	{
		return this->memory;
	}
}

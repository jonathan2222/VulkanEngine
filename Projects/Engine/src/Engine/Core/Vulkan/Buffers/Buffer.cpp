#include "stdafx.h"

#include "Buffer.h"
#include "../VulkanInstance.h"

namespace ym
{
	Buffer::Buffer() : buffer(VK_NULL_HANDLE), size(0)
	{

	}

	Buffer::~Buffer()
	{

	}

	void Buffer::init(VkDeviceSize size, VkBufferUsageFlags usage, const std::vector<uint32_t>& queueFamilyIndices)
	{
		this->size = size;

		VkBufferCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		createInfo.size = size;
		createInfo.usage = usage;

		if (queueFamilyIndices.size() > 1)
			createInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
		else
			createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		createInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
		createInfo.pQueueFamilyIndices = queueFamilyIndices.data();

		VULKAN_CHECK(vkCreateBuffer(VulkanInstance::get()->getLogicalDevice(), &createInfo, nullptr, &this->buffer), "Failed to create buffer");
	}

	void Buffer::destroy()
	{
		vkDestroyBuffer(VulkanInstance::get()->getLogicalDevice(), this->buffer, nullptr);
	}

	VkBuffer Buffer::getBuffer() const
	{
		return this->buffer;
	}

	VkMemoryRequirements Buffer::getMemReq() const
	{
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(VulkanInstance::get()->getLogicalDevice(), this->buffer, &memRequirements);

		return memRequirements;
	}

	VkDeviceSize Buffer::getSize() const
	{
		return this->size;
	}
}

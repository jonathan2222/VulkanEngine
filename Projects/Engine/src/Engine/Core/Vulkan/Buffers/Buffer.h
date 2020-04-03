#pragma once
#include "stdafx.h"

namespace ym
{
	class Memory;

	class Buffer
	{
	public:
		Buffer();
		~Buffer();

		void init(VkDeviceSize size, VkBufferUsageFlags usage, const std::vector<uint32_t>& queueFamilyIndices);
		void destroy();

		VkBuffer getBuffer() const;
		VkMemoryRequirements getMemReq() const;
		VkDeviceSize getSize() const;

	private:
		VkBuffer buffer;
		VkDeviceSize size;
	};
}
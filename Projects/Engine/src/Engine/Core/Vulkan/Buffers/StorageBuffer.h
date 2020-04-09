#pragma once

#include "Engine/Core/Vulkan/Buffers/Buffer.h"
#include "Engine/Core/Vulkan/Buffers/Memory.h"

namespace ym
{
	class StorageBuffer
	{
	public:
		void init(uint64_t size);
		void destroy();

		void transfer(void* data, uint64_t size, uint64_t offset);

		VkDescriptorBufferInfo getDescriptor() const;

	private:
		bool wasCreated{ false };
		Buffer buffer;
		Memory memory;
		VkDescriptorBufferInfo descriptor;
	};
}
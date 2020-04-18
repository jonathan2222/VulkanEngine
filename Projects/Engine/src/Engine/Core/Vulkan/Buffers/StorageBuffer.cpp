#include "stdafx.h"
#include "StorageBuffer.h"
#include "../Factory.h"

void ym::StorageBuffer::init(uint64_t size)
{
	if (!this->wasCreated)
	{
		std::vector<uint32_t> queueIndices = Factory::getQueueIndices(VK_QUEUE_GRAPHICS_BIT);
		this->buffer.init(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, queueIndices);
		this->memory.bindBuffer(&this->buffer);
		this->memory.init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		this->descriptor.buffer = this->buffer.getBuffer();
		this->descriptor.offset = (VkDeviceSize)0;
		this->descriptor.range = (VkDeviceSize)size;
	}
	this->wasCreated = true;
}

void ym::StorageBuffer::destroy()
{
	if (this->wasCreated)
	{
		this->buffer.destroy();
		this->memory.destroy();
		this->wasCreated = false;
	}
}

void ym::StorageBuffer::transfer(const void* data, uint64_t size, uint64_t offset)
{
	this->memory.directTransfer(&this->buffer, data, size, offset);
}

VkDescriptorBufferInfo ym::StorageBuffer::getDescriptor() const
{
	return this->descriptor;
}

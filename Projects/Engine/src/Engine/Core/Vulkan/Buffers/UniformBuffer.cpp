#include "stdafx.h"
#include "UniformBuffer.h"
#include "../Factory.h"

void ym::UniformBuffer::init(uint64_t size)
{
	if (!this->wasCreated)
	{
		std::vector<uint32_t> queueIndices = Factory::getQueueIndices(VK_QUEUE_GRAPHICS_BIT);
		this->buffer.init(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queueIndices);
		this->memory.bindBuffer(&this->buffer);
		this->memory.init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		this->descriptor.buffer = this->buffer.getBuffer();
		this->descriptor.offset = (VkDeviceSize)0;
		this->descriptor.range = (VkDeviceSize)size;
	}
	this->wasCreated = true;
}

void ym::UniformBuffer::destroy()
{
	if (this->wasCreated)
	{
		this->buffer.destroy();
		this->memory.destroy();
		this->wasCreated = false;
	}
}

void ym::UniformBuffer::transfer(const void* data, uint64_t size, uint64_t offset)
{
	this->memory.directTransfer(&this->buffer, data, size, offset);
}

VkDescriptorBufferInfo ym::UniformBuffer::getDescriptor() const
{
	return this->descriptor;
}

bool ym::UniformBuffer::isInitialized() const
{
	return this->wasCreated;
}

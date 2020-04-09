#pragma once

#include "stdafx.h"
#include "Texture.h"
#include "Buffers/Buffer.h"
#include "Buffers/Memory.h"
#include "VulkanInstance.h"
#include "Sampler.h"
#include "Texture.h"

namespace ym
{
	struct Factory
	{
		static std::vector<uint32_t> getQueueIndices(VkQueueFlagBits queues)
		{
			std::vector<uint32_t> queueIndices;
			if (queues & VK_QUEUE_GRAPHICS_BIT) queueIndices.push_back(VulkanInstance::get()->getGraphicsQueue().queueIndex);
			if (queues & VK_QUEUE_COMPUTE_BIT) queueIndices.push_back(VulkanInstance::get()->getComputeQueue().queueIndex);
			if (queues & VK_QUEUE_TRANSFER_BIT) queueIndices.push_back(VulkanInstance::get()->getTransferQueue().queueIndex);
			return queueIndices;
		}

		static void applyTextureDescriptor(Texture* texture, Sampler* sampler)
		{
			texture->descriptor.imageLayout = texture->image.getLayout();
			texture->descriptor.imageView = texture->imageView.getImageView();
			texture->descriptor.sampler = sampler->getSampler();
		}

		/*
			Creates one texture with its own device memory. (This does not set its data, use transferData to do that)
		*/
		static Texture* createTexture(TextureDesc textureDesc, VkImageUsageFlags usage, VkQueueFlagBits queues, VkImageAspectFlags aspectFlags)
		{
			std::vector<uint32_t> queueIndices = getQueueIndices(queues);
			Texture* texture = new Texture();
			texture->textureDesc = textureDesc;
			texture->image.init(textureDesc.width, textureDesc.height, textureDesc.format, usage, queueIndices, 0, 1);
			texture->memory = new Memory();
			texture->memory->bindTexture(texture);
			texture->memory->init(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			texture->imageView.init(texture->image.getImage(), VK_IMAGE_VIEW_TYPE_2D, textureDesc.format, aspectFlags, 1);
			return texture;
		}

		/*
			Create a staging memory and transfer the data to the device memory which lies in the texture.
		*/
		static void transferData(Texture* texture, CommandPool* transferCommandPool)
		{
			Buffer stagingBuffer;
			Memory stagingMemory;
			VkDeviceSize dataSize = (VkDeviceSize)SIZE_OF_ARR(texture->textureDesc.data);
			stagingBuffer.init(dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, { VulkanInstance::get()->getGraphicsQueue().queueIndex });
			stagingMemory.bindBuffer(&stagingBuffer);
			stagingMemory.init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			stagingMemory.directTransfer(&stagingBuffer, (void*)texture->textureDesc.data, dataSize, 0);

			// Setup a buffer copy region for the transfer.
			std::vector<VkBufferImageCopy> bufferCopyRegions;
			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = 0;
			bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = texture->textureDesc.width;
			bufferCopyRegion.imageExtent.height = texture->textureDesc.height;
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = 0;
			bufferCopyRegions.push_back(bufferCopyRegion);

			// Transfer data to texture memory.
			Image::TransistionDesc desc;
			desc.format = texture->textureDesc.format;
			desc.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			desc.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			desc.pool = transferCommandPool;
			desc.layerCount = 1;
			Image & image = texture->image;
			image.transistionLayout(desc);
			image.copyBufferToImage(&stagingBuffer, transferCommandPool, bufferCopyRegions);
			desc.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			desc.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			image.transistionLayout(desc);

			stagingBuffer.destroy();
			stagingMemory.destroy();
		}
	};
}
#pragma once

#include "stdafx.h"
#include "Texture.h"
#include "Buffers/Buffer.h"
#include "Buffers/Memory.h"
#include "VulkanInstance.h"
#include "Sampler.h"
#include "Texture.h"
#include "VulkanHelperfunctions.h"
#include "Engine/Core/Application/LayerManager.h"
#include "CommandBuffer.h"

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
			The resulting image will be in VK_IMAGE_LAYOUT_UNDEFINED. 
			!!!! If mipmaps is used, set usage to VK_IMAGE_USAGE_TRANSFER_SRC_BIT and VK_IMAGE_USAGE_TRANSFER_DST_BIT !!!
		*/
		static Texture* createTexture(TextureDesc textureDesc, VkImageUsageFlags usage, VkQueueFlagBits queues, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
		{
			std::vector<uint32_t> queueIndices = getQueueIndices(queues);
			Texture* texture = new Texture();
			texture->textureDesc = textureDesc;
			texture->image.init(textureDesc.width, textureDesc.height, textureDesc.format, usage, queueIndices, 0, 1, mipLevels);
			texture->memory = new Memory();
			texture->memory->bindTexture(texture);
			texture->memory->init(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			texture->imageView.init(texture->image.getImage(), VK_IMAGE_VIEW_TYPE_2D, textureDesc.format, aspectFlags, 1, mipLevels);
			return texture;
		}

		/*
			Create a staging memory and transfer the data to the device memory which lies in the texture.
		*/
		static void transferData(Texture* texture, CommandPool* transferCommandPool)
		{
			Buffer stagingBuffer;
			Memory stagingMemory;
			VkDeviceSize dataSize = getSizeFromFormat(texture->textureDesc.format)*texture->textureDesc.width*texture->textureDesc.height;
			stagingBuffer.init(dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, { VulkanInstance::get()->getGraphicsQueue().queueIndex });
			stagingMemory.bindBuffer(&stagingBuffer);
			stagingMemory.init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			stagingMemory.directTransfer(&stagingBuffer, (void*)texture->textureDesc.data, dataSize, 0);

			// Setup a buffer copy region for the transfer.
			// Only copy first mip level, the others will be generated later.
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
			stagingBuffer.destroy();
			stagingMemory.destroy();

			uint32_t mipMaps = texture->image.getMipLevels();
			if (mipMaps == 1)
			{
				desc.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				desc.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				image.transistionLayout(desc);
			}
			else
			{
				generateMipmaps(texture);
			}

		}

		/*
			Creates one texture with its own device memory. (This does not set its data, use transferData to do that)
		*/
		static Texture* createCubeMapTexture(TextureDesc textureDesc, VkImageUsageFlags usage, VkQueueFlagBits queues, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
		{
			const uint32_t numFaces = 6;
			std::vector<uint32_t> queueIndices = getQueueIndices(queues);
			Texture* texture = new Texture();
			texture->textureDesc = textureDesc;
			texture->image.init(textureDesc.width, textureDesc.height, textureDesc.format, usage, queueIndices, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, numFaces, mipLevels);
			texture->memory = new Memory();
			texture->memory->bindTexture(texture);
			texture->memory->init(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			texture->imageView.init(texture->image.getImage(), VK_IMAGE_VIEW_TYPE_CUBE, textureDesc.format, aspectFlags, numFaces, mipLevels);
			return texture;
		}

		/*
			Create a staging memory and transfer the data to the device memory which lies in the cube map texture.
			The data in texture must have all the sides in the order right, left, top, bottom, front, back.
		*/
		static void transferCubeMapData(Texture* texture, CommandPool* transferCommandPool)
		{
			const uint32_t numFaces = 6;
			VkDeviceSize size = (VkDeviceSize)texture->textureDesc.width * (VkDeviceSize)texture->textureDesc.height * getSizeFromFormat(texture->textureDesc.format);

			Buffer stagingBuffer;
			Memory stagingMemory;
			stagingBuffer.init(size * numFaces, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, { VulkanInstance::get()->getGraphicsQueue().queueIndex });
			stagingMemory.bindBuffer(&stagingBuffer);
			stagingMemory.init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			// Transfer the data to the buffer.
			stagingMemory.directTransfer(&stagingBuffer, texture->textureDesc.data, size * numFaces, 0);

			// Setup buffer copy regions for each face including all of its miplevels.
			std::vector<VkBufferImageCopy> bufferCopyRegions;
			for (uint32_t face = 0; face < numFaces; face++)
			{
				for (uint32_t level = 0; level < texture->image.getMipLevels(); level++)
				{
					VkBufferImageCopy bufferCopyRegion = {};
					bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					bufferCopyRegion.imageSubresource.mipLevel = level;
					bufferCopyRegion.imageSubresource.baseArrayLayer = face;
					bufferCopyRegion.imageSubresource.layerCount = 1;
					bufferCopyRegion.imageExtent.width = texture->textureDesc.width >> level;
					bufferCopyRegion.imageExtent.height = texture->textureDesc.height >> level;
					bufferCopyRegion.imageExtent.depth = 1;
					bufferCopyRegion.bufferOffset = (VkDeviceSize)(face * size);
					bufferCopyRegions.push_back(bufferCopyRegion);
				}
			}

			// Transfer data to texture memory.
			Image::TransistionDesc desc;
			desc.format = texture->textureDesc.format;
			desc.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			desc.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			desc.pool = transferCommandPool;
			desc.layerCount = numFaces;
			Image & image = texture->image;
			image.transistionLayout(desc);
			image.copyBufferToImage(&stagingBuffer, transferCommandPool, bufferCopyRegions);

			if (texture->image.getMipLevels() == 1)
			{
				desc.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				desc.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				image.transistionLayout(desc);
			}

			stagingBuffer.destroy();
			stagingMemory.destroy();
		}

		/*
			Generates mipmaps for the specified texture. The image needs to have the VK_IMAGE_USAGE_TRANSFER_SRC_BIT and VK_IMAGE_USAGE_TRANSFER_DST_BIT and be in
			the layout VK_IMAGE_LAYOUT_DST_OPTIMAL. After this function, the image will be in VK_IMAGE_LAYOUT_SHADER_READ_ONLY_BIT.
		*/
		static void generateMipmaps(Texture* texture)
		{
			CommandPool* pool = &LayerManager::get()->getCommandPools()->graphicsPool;
			CommandBuffer* commandBuffer = pool->beginSingleTimeCommand();

			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = texture->image.getImage();
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.layerCount = 1;
			barrier.subresourceRange.levelCount = 1;

			uint32_t mipWidth = texture->textureDesc.width;
			uint32_t mipHeight = texture->textureDesc.height;
			for (uint32_t j = 0; j < texture->image.getArrayLayers(); j++)
			{
				barrier.subresourceRange.baseArrayLayer = j;

				for (uint32_t i = 1; i < texture->image.getMipLevels(); i++)
				{
					barrier.subresourceRange.baseMipLevel = i - 1;
					barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
					barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

					vkCmdPipelineBarrier(commandBuffer->getCommandBuffer(),
						VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
						0, nullptr, 0, nullptr, 1, &barrier);

					VkImageBlit blit = {};
					blit.srcOffsets[0] = { 0, 0, 0 };
					blit.srcOffsets[1] = { (int32_t)mipWidth, (int32_t)mipHeight, 1 };
					blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					blit.srcSubresource.mipLevel = i - 1;
					blit.srcSubresource.baseArrayLayer = j;
					blit.srcSubresource.layerCount = 1;
					blit.dstOffsets[0] = { 0, 0, 0 };
					blit.dstOffsets[1] = { (int32_t)mipWidth > 1 ? (int32_t)mipWidth / 2 : 1, (int32_t)mipHeight > 1 ? (int32_t)mipHeight / 2 : 1, 1 };
					blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					blit.dstSubresource.mipLevel = i;
					blit.dstSubresource.baseArrayLayer = j;
					blit.dstSubresource.layerCount = 1;
					vkCmdBlitImage(commandBuffer->getCommandBuffer(),
						texture->image.getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						texture->image.getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						1, &blit, VK_FILTER_LINEAR);

					// Transition to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
					barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
					barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
					barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
					vkCmdPipelineBarrier(commandBuffer->getCommandBuffer(),
						VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
						0, nullptr, 0, nullptr, 1, &barrier);

					if (mipWidth > 1) mipWidth /= 2;
					if (mipHeight > 1) mipHeight /= 2;
				}

				barrier.subresourceRange.baseMipLevel = texture->image.getMipLevels() - 1;
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				vkCmdPipelineBarrier(commandBuffer->getCommandBuffer(),
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
					0, nullptr, 0, nullptr, 1, &barrier);
			}

			texture->image.setLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			pool->endSingleTimeCommand(commandBuffer);
		}
	};
}
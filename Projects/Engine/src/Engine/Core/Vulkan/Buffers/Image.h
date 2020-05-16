#pragma once

#include <vulkan/vulkan.h>

namespace ym
{

	class Buffer;
	class CommandPool;
	class CommandBuffer;

	class Image
	{
	public:
		struct TransistionDesc
		{
			VkFormat format;
			VkImageLayout oldLayout;
			VkImageLayout newLayout;
			CommandPool* pool;
			uint32_t layerCount = 1;
		};

	public:
		Image();
		~Image();

		void init(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, const std::vector<uint32_t>& queueFamilyIndices, VkImageCreateFlags flags, uint32_t arrayLayers, uint32_t mipLevels);
		void destroy();

		void transistionLayout(TransistionDesc& desc);
		void setLayout(CommandBuffer* cmdBuff, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout);
		void setLayout(CommandBuffer* cmdBuff, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkImageSubresourceRange subresourceRange);
		// TODO: REMOVE THIS!!
		void setLayout(VkImageLayout newImageLayout);
		void copyBufferToImage(Buffer* buffer, CommandPool* pool);
		void copyBufferToImage(Buffer* buffer, CommandPool* pool, std::vector<VkBufferImageCopy> regions);

		VkImage getImage() const;
		VkImageLayout getLayout() const { return this->layout; }
		uint32_t getMipLevels() const;
		uint32_t getArrayLayers() const;

	private:
		VkImage image;
		VkImageLayout layout;

		uint32_t width;
		uint32_t height;
		uint32_t mipLevels;
		uint32_t layers;
	};
}
#pragma once

#include <vulkan/vulkan.h>

namespace ym
{
	class ImageView
	{
	public:
		ImageView();
		~ImageView();

		void init(VkImage image, VkImageViewType type, VkFormat format, VkImageAspectFlags aspectMask, uint32_t layerCount, uint32_t mipLevels);
		void destroy();

		VkImageView getImageView() const { return this->imageView; }

	private:
		VkImageView imageView;
	};
}
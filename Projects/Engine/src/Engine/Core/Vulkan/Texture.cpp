#include "stdafx.h"
#include "Texture.h"
#include "VulkanInstance.h"

namespace ym
{
	Texture::Texture() : width(0), height(0), format(VK_FORMAT_R8G8B8A8_UNORM)
	{
	}

	Texture::~Texture()
	{
	}

	void Texture::init(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, const std::vector<uint32_t>& queueFamilyIndices, VkImageCreateFlags flags, uint32_t arrayLayers)
	{
		this->width = width;
		this->height = height;
		this->format = format;
		this->image.init(width, height, format, usage, queueFamilyIndices, flags, arrayLayers);
		//this->imageView.init(getVkImage(), VK_IMAGE_VIEW_TYPE_2D, format);
	}

	void Texture::destroy()
	{
		this->imageView.destroy();
		this->image.destroy();
	}

	VkMemoryRequirements Texture::getMemReq() const
	{
		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(VulkanInstance::get()->getLogicalDevice(), this->image.getImage(), &memRequirements);

		return memRequirements;
	}

	VkImage Texture::getVkImage() const
	{
		return this->image.getImage();
	}

	Image& Texture::getImage()
	{
		return this->image;
	}

	VkImageView Texture::getVkImageView() const
	{
		return this->imageView.getImageView();
	}

	VkFormat Texture::getFormat() const
	{
		return this->format;
	}

	uint32_t Texture::getWidth() const
	{
		return this->width;
	}

	uint32_t Texture::getHeight() const
	{
		return this->height;
	}

	ImageView& Texture::getImageView()
	{
		return this->imageView;
	}
}

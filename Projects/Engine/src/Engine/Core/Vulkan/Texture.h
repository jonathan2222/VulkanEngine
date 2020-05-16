#pragma once

#include "stdafx.h"
#include "Buffers/Image.h"
#include "Buffers/ImageView.h"
#include "Buffers/Memory.h"
#include "VulkanInstance.h"

namespace ym
{
	struct TextureDesc
	{
		uint32_t width;
		uint32_t height;
		VkFormat format;
		void* data = nullptr; // This is not guaranteed to always be set.
	};

	struct Texture
	{
		Image image;
		ImageView imageView;
		TextureDesc textureDesc;
		VkDescriptorImageInfo descriptor;
		Memory* memory = nullptr; // Optional, can use another memory if needed.

		VkMemoryRequirements Texture::getMemoryRequirements() const
		{
			VkMemoryRequirements memRequirements;
			vkGetImageMemoryRequirements(VulkanInstance::get()->getLogicalDevice(), this->image.getImage(), &memRequirements);
			return memRequirements;
		}

		void destroy()
		{
			this->image.destroy();
			this->imageView.destroy();
			this->textureDesc = {};
			this->descriptor = {};
			if (this->memory) this->memory->destroy();
			SAFE_DELETE(this->memory);
		}
	};
}
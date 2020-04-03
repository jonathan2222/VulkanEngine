#pragma once

#include "stdafx.h"

namespace ym
{
	class RenderPass;

	class Framebuffer
	{
	public:
		Framebuffer();
		~Framebuffer();

		void init(size_t numFrameBuffers, RenderPass* renderpass, const std::vector<VkImageView>& attachments, VkExtent2D extent);

		VkFramebuffer getFramebuffer();

		void destroy();

	private:
		VkFramebuffer framebuffer;
	};
}

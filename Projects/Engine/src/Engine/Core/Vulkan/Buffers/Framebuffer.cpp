#include "stdafx.h"
#include "Framebuffer.h"

#include "../Pipeline/RenderPass.h"
#include "../VulkanInstance.h"

namespace ym
{
	Framebuffer::Framebuffer()
	{
	}

	Framebuffer::~Framebuffer()
	{
	}

	void Framebuffer::init(size_t numFrameBuffers, RenderPass* renderpass, const std::vector<VkImageView>& attachments, VkExtent2D extent)
	{
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderpass->getRenderPass();
		framebufferInfo.width = extent.width;
		framebufferInfo.height = extent.height;
		framebufferInfo.layers = 1;

		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();

		VULKAN_CHECK(vkCreateFramebuffer(VulkanInstance::get()->getLogicalDevice(), &framebufferInfo, nullptr, &this->framebuffer), "Failed to create framebuffer");
	}

	VkFramebuffer Framebuffer::getFramebuffer()
	{
		return this->framebuffer;
	}

	void Framebuffer::destroy()
	{
		vkDestroyFramebuffer(VulkanInstance::get()->getLogicalDevice(), this->framebuffer, nullptr);
	}
}

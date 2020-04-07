#include "stdafx.h"
#include "Renderer.h"

#include "../Input/Config.h"
#include "Engine/Core/Vulkan/Factory.h"
#include "Engine/Core/Scene/GLTFLoader.h"
#include "Engine/Core/Application/LayerManager.h"

ym::Renderer::Renderer()
{
	this->depthTexture = nullptr;
}

ym::Renderer::~Renderer()
{
}

void ym::Renderer::init()
{
	int width = Config::get()->fetch<int>("Display/defaultWidth");
	int height = Config::get()->fetch<int>("Display/defaultHeight");
	this->swapChain.init(width, height);

	createDepthTexture();
	createRenderPass();
	createFramebuffers(this->depthTexture->imageView.getImageView());

	this->modelRenderer.init(&this->swapChain);

	GLTFLoader::initDefaultData(&LayerManager::get()->getCommandPools()->graphicsPool);
}

void ym::Renderer::destroy()
{
	GLTFLoader::destroyDefaultData();

	this->modelRenderer.destroy();

	this->renderPass.destroy();
	this->depthTexture->destroy();
	SAFE_DELETE(this->depthTexture);
	for (Framebuffer& framebuffer : this->framebuffers) framebuffer.destroy();
	this->swapChain.destory();
}

void ym::Renderer::createDepthTexture()
{
	TextureDesc textureDesc;
	textureDesc.width = this->swapChain.getExtent().width;
	textureDesc.height = this->swapChain.getExtent().height;
	textureDesc.format = findDepthFormat(VulkanInstance::get()->getPhysicalDevice());
	textureDesc.data = nullptr;
	this->depthTexture = ym::Factory::createTexture(textureDesc, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_QUEUE_GRAPHICS_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);

	// Transistion image
	Image::TransistionDesc desc;
	desc.format = textureDesc.format;
	desc.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	desc.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	desc.pool = &LayerManager::get()->getCommandPools()->graphicsPool;
	this->depthTexture->image.transistionLayout(desc);
}

void ym::Renderer::createRenderPass()
{
	this->renderPass.addDefaultColorAttachment(this->swapChain.getImageFormat());
	this->renderPass.addDefaultDepthAttachment();

	RenderPass::SubpassInfo subpassInfo;
	subpassInfo.colorAttachmentIndices = { 0 }; // One color attachment
	subpassInfo.depthStencilAttachmentIndex = 1; // Depth attachment
	this->renderPass.addSubpass(subpassInfo);

	VkSubpassDependency subpassDependency = {};
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.dstSubpass = 0;
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.srcAccessMask = 0;
	subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	this->renderPass.addSubpassDependency(subpassDependency);
	this->renderPass.init();
}

void ym::Renderer::createFramebuffers(VkImageView depthAttachment)
{
	this->framebuffers.resize(this->swapChain.getNumImages());
	for (size_t i = 0; i < this->swapChain.getNumImages(); i++)
	{
		std::vector<VkImageView> imageViews;
		imageViews.push_back(this->swapChain.getImageViews()[i]);  // Add color attachment
		imageViews.push_back(depthAttachment); // Add depth image view
		this->framebuffers[i].init(this->swapChain.getNumImages(), &this->renderPass, imageViews, this->swapChain.getExtent());
	}
}

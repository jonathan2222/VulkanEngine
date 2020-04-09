#include "stdafx.h"
#include "Renderer.h"

#include "../Input/Config.h"
#include "Engine/Core/Vulkan/Factory.h"
#include "Engine/Core/Scene/GLTFLoader.h"
#include "Engine/Core/Application/LayerManager.h"
#include "Engine/Core/Threading/ThreadManager.h"

ym::Renderer::Renderer()
{
	this->depthTexture = nullptr;
	this->currentFrame = 0;
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

	// Each renderer has a corresponding thread.
	ThreadManager::init((uint32_t)ERendererType::RENDER_TYPE_SIZE);
	this->modelRenderer.init(&this->swapChain, (uint32_t)ERendererType::RENDER_TYPE_MODEL, &this->renderPass); // Uses thread 0.

	CommandPool& graphicsPool = LayerManager::get()->getCommandPools()->graphicsPool;
	GLTFLoader::initDefaultData(&graphicsPool);

	createSyncObjects();

	this->primaryCommandBuffers = graphicsPool.createCommandBuffers(this->swapChain.getNumImages(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}

void ym::Renderer::destroy()
{
	ThreadManager::destroy();

	GLTFLoader::destroyDefaultData();

	this->modelRenderer.destroy();

	this->renderPass.destroy();
	this->depthTexture->destroy();
	SAFE_DELETE(this->depthTexture);
	for (Framebuffer& framebuffer : this->framebuffers) framebuffer.destroy();
	destroySyncObjects();
	this->swapChain.destory();
}

void ym::Renderer::setCamera(Camera* camera)
{
	this->modelRenderer.setCamera(camera);
}

bool ym::Renderer::begin()
{
	vkWaitForFences(VulkanInstance::get()->getLogicalDevice(), 1, &this->inFlightFences[this->currentFrame], VK_TRUE, UINT64_MAX);
	VkResult result = vkAcquireNextImageKHR(VulkanInstance::get()->getLogicalDevice(), this->swapChain.getSwapChain(), UINT64_MAX, this->imageAvailableSemaphores[this->currentFrame], VK_NULL_HANDLE, &this->imageIndex);

	// Check if window has been resized.
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		return false;
	}
	YM_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Failed to aquire swap chain image!");

	// Check if a previous frame is using this image (Wait for this image to be free for use).
	if (this->imagesInFlight[this->imageIndex] != VK_NULL_HANDLE) {
		vkWaitForFences(VulkanInstance::get()->getLogicalDevice(), 1, &this->imagesInFlight[this->imageIndex], VK_TRUE, UINT64_MAX);
	}

	/*
		Marked image as being used by this frame.
		This current frame will use the image with index imageIndex, mark this so that we now wen we can use this again.
	*/
	this->imagesInFlight[this->imageIndex] = this->inFlightFences[this->currentFrame];

	// Prepare inheritance info for the secondary buffers.
	this->inheritanceInfo = {};
	this->inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	this->inheritanceInfo.framebuffer = this->framebuffers[this->imageIndex].getFramebuffer();
	this->inheritanceInfo.renderPass = this->renderPass.getRenderPass();

	this->modelRenderer.begin(this->inheritanceInfo);

	return true;
}

void ym::Renderer::drawModel(Model* model, const glm::mat4& transform)
{
	// Add model for drawing.
	this->modelRenderer.drawModel(model, transform);
}

bool ym::Renderer::end()
{
	this->modelRenderer.end(this->imageIndex);

	submit();

	VkSemaphore waitSemaphores[] = { this->renderFinishedSemaphores[this->currentFrame] };
	VkSwapchainKHR swapchain = this->swapChain.getSwapChain();

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = waitSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.pImageIndices = &this->imageIndex;

	VkResult result = vkQueuePresentKHR(VulkanInstance::get()->getPresentQueue().queue, &presentInfo);

	// Check if the window was resized.
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		return false;
	}
	YM_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Failed to present image!");

	this->currentFrame = (this->currentFrame + 1) % this->framesInFlight;
	return true;
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

void ym::Renderer::createSyncObjects()
{
	this->framesInFlight = 3;
	this->imageAvailableSemaphores.resize(this->framesInFlight);
	this->renderFinishedSemaphores.resize(this->framesInFlight);
	this->inFlightFences.resize(this->framesInFlight);
	this->imagesInFlight.resize(this->swapChain.getNumImages(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo SemaCreateInfo = {};
	SemaCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (uint32_t i = 0; i < this->framesInFlight; i++) {
		VULKAN_CHECK(vkCreateSemaphore(VulkanInstance::get()->getLogicalDevice(), &SemaCreateInfo, nullptr, &this->imageAvailableSemaphores[i]), "Failed to create image available semaphores");
		VULKAN_CHECK(vkCreateSemaphore(VulkanInstance::get()->getLogicalDevice(), &SemaCreateInfo, nullptr, &this->renderFinishedSemaphores[i]), "Failed to create render finished semaphores");
		VULKAN_CHECK(vkCreateFence(VulkanInstance::get()->getLogicalDevice(), &fenceCreateInfo, nullptr, &this->inFlightFences[i]), "Failed to create in flight fences");
	}
}

void ym::Renderer::destroySyncObjects()
{
	for (uint32_t i = 0; i < this->framesInFlight; i++) {
		vkDestroySemaphore(VulkanInstance::get()->getLogicalDevice(), this->imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(VulkanInstance::get()->getLogicalDevice(), this->renderFinishedSemaphores[i], nullptr);
		vkDestroyFence(VulkanInstance::get()->getLogicalDevice(), this->inFlightFences[i], nullptr);
	}
}

void ym::Renderer::submit()
{
	// Wait for model renderer to finish.
	//ThreadManager::wait((uint32_t)ERendererType::RENDER_TYPE_MODEL); // Wait on ModelRenderer

	// Gather all secondary buffers. (One in this case)
	std::vector<CommandBuffer*>& secondaryBuffers = this->modelRenderer.getBuffers();
	std::vector<VkCommandBuffer> vkCommands;
	vkCommands.push_back(secondaryBuffers[this->imageIndex]->getCommandBuffer());

	// Record and execute them on the current primary command buffer.
	CommandBuffer* buffer = this->primaryCommandBuffers[this->imageIndex];
	buffer->begin(0, nullptr);
	std::vector<VkClearValue> clearValues = {};
	VkClearValue value;
	value.color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues.push_back(value);
	value.depthStencil = { 1.0f, 0 };
	clearValues.push_back(value);
	buffer->cmdBeginRenderPass(&this->renderPass, this->framebuffers[this->imageIndex].getFramebuffer(), this->swapChain.getExtent(), clearValues, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
	buffer->cmdExecuteCommands((uint32_t)vkCommands.size(), vkCommands.data());
	buffer->cmdEndRenderPass();
	buffer->end();

	// Submit commands to GPU.
	const VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };
	std::vector<VkSemaphore> waitSemaphores = { this->imageAvailableSemaphores[this->currentFrame] };
	VkSemaphore signalSemaphores[] = { this->renderFinishedSemaphores[this->currentFrame] };
	std::vector<VkCommandBuffer> buffers = { buffer->getCommandBuffer() };

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.waitSemaphoreCount = (uint32_t)waitSemaphores.size();
	submitInfo.pWaitSemaphores = waitSemaphores.data();
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;
	submitInfo.pCommandBuffers = buffers.data();
	submitInfo.commandBufferCount = (uint32_t)buffers.size();

	vkResetFences(VulkanInstance::get()->getLogicalDevice(), 1, &this->inFlightFences[this->currentFrame]);
	VULKAN_CHECK(vkQueueSubmit(VulkanInstance::get()->getGraphicsQueue().queue, 1, &submitInfo, this->inFlightFences[this->currentFrame]), "Failed to sumbit commandbuffer!");

}

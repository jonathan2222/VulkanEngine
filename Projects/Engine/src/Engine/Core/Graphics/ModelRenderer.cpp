#include "stdafx.h"
#include "ModelRenderer.h"

ym::ModelRenderer::ModelRenderer()
{
	this->framesInFlight = 0;
	this->currentFrame = 0;
	this->imageIndex = 0;
	this->numImages = 0;
}

ym::ModelRenderer::~ModelRenderer()
{
}

ym::ModelRenderer* ym::ModelRenderer::get()
{
	static ModelRenderer modelRenderer;
	return &modelRenderer;
}

void ym::ModelRenderer::init(SwapChain* swapChain)
{
	this->swapChain = swapChain;

	this->commandPool.init(CommandPool::Queue::GRAPHICS, 0);
	this->commandBuffers = this->commandPool.createCommandBuffers(this->swapChain->getNumImages(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	createSyncObjects();
}

void ym::ModelRenderer::destroy()
{
	this->commandPool.destroy();

	destroySyncObjects();
}

bool ym::ModelRenderer::begin()
{
	vkWaitForFences(VulkanInstance::get()->getLogicalDevice(), 1, &this->inFlightFences[this->currentFrame], VK_TRUE, UINT64_MAX);
	VkResult result = vkAcquireNextImageKHR(VulkanInstance::get()->getLogicalDevice(), this->swapChain->getSwapChain(), UINT64_MAX, this->imageAvailableSemaphores[this->currentFrame], VK_NULL_HANDLE, &this->imageIndex);

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

	return true;
}

void ym::ModelRenderer::drawModel(Model* model)
{
	// Record to command buffer.


	// Submit the buffer to the GPU.
	const VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };
	std::vector<VkSemaphore> waitSemaphores = { this->imageAvailableSemaphores[this->currentFrame] };
	VkSemaphore signalSemaphores[] = { this->renderFinishedSemaphores[this->currentFrame] };
	std::vector<VkCommandBuffer> buffers = { commandBuffers[this->imageIndex]->getCommandBuffer() };
	
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.waitSemaphoreCount = waitSemaphores.size();
	submitInfo.pWaitSemaphores = waitSemaphores.data();
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;
	submitInfo.pCommandBuffers = buffers.data();
	submitInfo.commandBufferCount = (uint32_t)buffers.size();

	vkResetFences(VulkanInstance::get()->getLogicalDevice(), 1, &this->inFlightFences[this->currentFrame]);
	VULKAN_CHECK(vkQueueSubmit(VulkanInstance::get()->getGraphicsQueue().queue, 1, &submitInfo, this->inFlightFences[this->currentFrame]), "Failed to sumbit commandbuffer!");
}

bool ym::ModelRenderer::end()
{
	VkSemaphore waitSemaphores[] = { this->renderFinishedSemaphores[this->currentFrame] };
	VkSwapchainKHR swapchain = this->swapChain->getSwapChain();

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

void ym::ModelRenderer::createSyncObjects()
{
	this->imageAvailableSemaphores.resize(this->framesInFlight);
	this->renderFinishedSemaphores.resize(this->framesInFlight);
	this->inFlightFences.resize(this->framesInFlight);
	this->imagesInFlight.resize(this->numImages, VK_NULL_HANDLE);

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

void ym::ModelRenderer::destroySyncObjects()
{
	for (uint32_t i = 0; i < this->framesInFlight; i++) {
		vkDestroySemaphore(VulkanInstance::get()->getLogicalDevice(), this->imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(VulkanInstance::get()->getLogicalDevice(), this->renderFinishedSemaphores[i], nullptr);
		vkDestroyFence(VulkanInstance::get()->getLogicalDevice(), this->inFlightFences[i], nullptr);
	}
}

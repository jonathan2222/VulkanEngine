#include "stdafx.h"
#include "SwapChain.h"

#include "VulkanInstance.h"

ym::SwapChain::SwapChain() : swapChain(VK_NULL_HANDLE)
{
}

ym::SwapChain::~SwapChain()
{
}

void ym::SwapChain::init(unsigned int width, unsigned int height)
{
	createSwapChain(width, height);
	fetchImages(this->numImages);
	createImageViews(this->numImages);
}

void ym::SwapChain::destory()
{
	VulkanInstance* instance = VulkanInstance::get();

	// Destory swap chain image views.
	for(VkImageView view : this->imageViews)
		vkDestroyImageView(instance->getLogicalDevice(), view, nullptr);
	this->imageViews.clear();

	// Destroy swap chain
	vkDestroySwapchainKHR(instance->getLogicalDevice(), this->swapChain, nullptr);
	this->swapChain = VK_NULL_HANDLE;
}

void ym::SwapChain::createSwapChain(unsigned int width, unsigned int height)
{
	VulkanInstance* instance = VulkanInstance::get();

	VulkanInstance::SwapChainSupportDetails swapChainSupport = instance->getSwapChainSupportDetails();

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, width, height);

	// This will probably be 2 or 3.
	this->numImages = calcNumSwapChainImages(swapChainSupport);

	// Define the swap chain.
	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = instance->getSurface();
	createInfo.minImageCount = this->numImages;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1; // Stereoscopic 3D application will not have a 1 here.
	// If you are rendering to a separate image first, for example post-process effects, then use VK_IMAGE_USAGE_TRANSFER_DST_BIT to transfer that image to the swap chain image.
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; 

	VulkanInstance::QueueFamilyIndices indices = instance->getQueueFamilies();
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	// Create swap chain.
	VkResult result = vkCreateSwapchainKHR(instance->getLogicalDevice(), &createInfo, nullptr, &this->swapChain);
	YM_ASSERT(result == VK_SUCCESS, "Failed to create swap chain!");

	this->imageFormat = surfaceFormat.format;
	this->extent = extent;
}

void ym::SwapChain::fetchImages(uint32_t imageCount)
{
	VulkanInstance* instance = VulkanInstance::get();

	// Fetch images from the swap chain.
	vkGetSwapchainImagesKHR(instance->getLogicalDevice(), this->swapChain, &imageCount, nullptr);
	this->images.resize(imageCount);
	vkGetSwapchainImagesKHR(instance->getLogicalDevice(), this->swapChain, &imageCount, this->images.data());
}

void ym::SwapChain::createImageViews(uint32_t imageCount)
{
	VulkanInstance* instance = VulkanInstance::get();

	// Create an image view for each image from the swap chain.
	this->imageViews.resize(imageCount);
	for (uint32_t i = 0; i < imageCount; i++) {
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = this->images[i];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = this->imageFormat;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		VkResult result = vkCreateImageView(instance->getLogicalDevice(), &viewInfo, nullptr, &imageView);
		YM_ASSERT(result == VK_SUCCESS, "Failed to create texture image view!");
		this->imageViews[i] = imageView;
	}
}

uint32_t ym::SwapChain::calcNumSwapChainImages(VulkanInstance::SwapChainSupportDetails& swapChainSupport)
{
	// Request one more image then the minimum number of images to avoid to wait on the driver to complete internal operations before we can acquire another image to render to.
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

	// If it exceeds the maximum number of images, than set it to the maximum number of images. 
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}
	
	return imageCount;
}

VkSurfaceFormatKHR ym::SwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}
	return availableFormats[0];
}

VkPresentModeKHR ym::SwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const auto& availablePresentMode : availablePresentModes) {
		// Try to find the mode for triple buffering.
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
	}
	// If not found, use the standard. (vsync)
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D ym::SwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, unsigned int width, unsigned int height)
{
	// If the window manager allow us to have a different resolution (width is set to UINT32_MAX) then try to match it to the window size. 
	if (capabilities.currentExtent.width != UINT32_MAX) {
		return capabilities.currentExtent;
	}
	else {
		VkExtent2D actualExtent = { width, height };
		actualExtent.width = max(capabilities.minImageExtent.width, min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = max(capabilities.minImageExtent.height, min(capabilities.maxImageExtent.height, actualExtent.height));
		return actualExtent;
	}
}

#pragma once

#include "stdafx.h"

#include "VulkanInstance.h"

namespace ym
{
	class SwapChain
	{
	public:
		SwapChain();
		virtual ~SwapChain();

		void init(unsigned int width, unsigned int height);
		void destory();

		uint32_t getNumImages() const;
		VkSwapchainKHR getSwapChain() const;
		VkExtent2D getExtent() const;
		VkFormat getImageFormat() const;
		std::vector<VkImageView> getImageViews() const;

	private:
		void createSwapChain(unsigned int width, unsigned int height);

		void fetchImages(uint32_t imageCount);
		void createImageViews(uint32_t imageCount);

		uint32_t calcNumSwapChainImages(SwapChainSupportDetails& swapChainSupport);
		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, unsigned int width, unsigned int height);

		VkSwapchainKHR swapChain;
		VkFormat imageFormat;
		VkExtent2D extent;

		uint32_t numImages;
		std::vector<VkImage> images;
		std::vector<VkImageView> imageViews;
	};
}
#pragma once

#include "stdafx.h"

#include "Engine/Core/Vulkan/VulkanHelperfunctions.h"

namespace ym
{
	struct VulkanQueue {
		VulkanQueue() : queue(VK_NULL_HANDLE), queueIndex(0) {};

		VkQueue queue;
		uint32_t queueIndex;
	};

	class VulkanInstance
	{
	public:
		virtual ~VulkanInstance();

		// Get the instance of the class Instance.
		static VulkanInstance* get();
		
		void init();
		void destroy();

		SwapChainSupportDetails getSwapChainSupportDetails();
		QueueFamilyIndices getQueueFamilies();

		VkInstance getInstance();
		VkSurfaceKHR getSurface();
		VkPhysicalDevice getPhysicalDevice();
		VkDevice getLogicalDevice();

		VulkanQueue getGraphicsQueue() const { return this->graphicsQueue; }
		VulkanQueue getPresentQueue() const { return this->presentQueue; }
		VulkanQueue getTransferQueue() const { return this->transferQueue; }
		VulkanQueue getComputeQueue() const { return this->computeQueue; }

	private:
		VulkanInstance();

		void createInstance(std::vector<const char*> additionalInstanceExtensions);
		std::vector<const char*> getRequiredExtensions(std::vector<const char*> additionalInstanceExtensions);

		void createSurface();

		void pickPhysicalDevice();
		int calculateDeviceSuitability(VkPhysicalDevice device);
		bool isDeviceSuitable(VkPhysicalDevice device, VkPhysicalDeviceFeatures& deviceFeatures);

		void createLogicalDevice(VkPhysicalDeviceFeatures deviceFeatures);

		// Utils functions
		bool checkDeviceExtensionSupport(VkPhysicalDevice device);
		bool checkValidationLayerSupport(const std::vector<const char*>& validationLayers);

		// Debug functions
		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, 
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
		VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
			const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
		void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugInfo);

	private:
		static std::vector<const char*> deviceExtensions;
		
		bool enableValidationLayers;
		static std::vector<const char*> validationLayers;

		VkInstance instance;
		VkDebugUtilsMessengerEXT debugMessenger;
		VkSurfaceKHR surface;

		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;

		VulkanQueue graphicsQueue;
		VulkanQueue presentQueue;
		VulkanQueue transferQueue;
		VulkanQueue computeQueue;
	};
}
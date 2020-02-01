#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

namespace ym
{
	class VulkanInstance
	{
	public:
		virtual ~VulkanInstance();

		// Get the instance of the class Instance.
		static VulkanInstance* get();
		
		void init();
		void destroy();

		VkInstance getInstance();
		VkSurfaceKHR getSurface();

		VkPhysicalDevice getPhysicalDevice();
		VkDevice getLogicalDevice();

		VkQueue getPresentQueue();
		VkQueue getGraphicsQueue();

	private:
		struct QueueFamilyIndices {
			std::optional<unsigned> graphicsFamily;
			std::optional<uint32_t> presentFamily;

			bool isComplete() {
				return graphicsFamily.has_value() && presentFamily.has_value();
			}
		};

		struct SwapChainSupportDetails {
			SwapChainSupportDetails(SwapChainSupportDetails& other) = default;
			VkSurfaceCapabilitiesKHR capabilities;
			std::vector<VkSurfaceFormatKHR> formats;
			std::vector<VkPresentModeKHR> presentModes;
		};

		VulkanInstance();

		void createInstance(std::vector<const char*> additionalInstanceExtensions);
		std::vector<const char*> getRequiredExtensions(std::vector<const char*> additionalInstanceExtensions);

		void createSurface();

		void pickPhysicalDevice();
		int calculateDeviceSuitability(VkPhysicalDevice device);
		bool isDeviceSuitable(VkPhysicalDevice device, VkPhysicalDeviceFeatures& deviceFeatures);

		void createLogicalDevice(VkPhysicalDeviceFeatures deviceFeatures);

		// Utils functions
		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
		bool checkDeviceExtensionSupport(VkPhysicalDevice device);
		bool checkValidationLayerSupport(const std::vector<const char*>& validationLayers);

		// Debug functions
		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, 
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
		VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
			const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
		void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugInfo);

		// -------------------------------------------- Member variables --------------------------------------------

		static std::vector<const char*> deviceExtensions;
		
		bool enableValidationLayers;
		static std::vector<const char*> validationLayers;

		VkInstance instance;
		VkDebugUtilsMessengerEXT debugMessenger;
		VkSurfaceKHR surface;

		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;

		VkQueue graphicsQueue;
		VkQueue presentQueue;
	};
}
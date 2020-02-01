#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace ym
{
	class VulkanInstance
	{
	public:
		virtual ~VulkanInstance();

		// Get the instance of the class Instance.
		static VulkanInstance* get();
		
		void init(std::vector<const char*> additionalDeviceExtensions, std::vector<const char*> additionalValidationLayers);
		void destroy();

	private:
		VulkanInstance();

		void createInstance();
		std::vector<const char*> getRequiredExtensions(std::vector<const char*> additionalExtensions);

		bool checkValidationLayerSupport(const std::vector<const char*>& validationLayers);

		void createSurface();

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
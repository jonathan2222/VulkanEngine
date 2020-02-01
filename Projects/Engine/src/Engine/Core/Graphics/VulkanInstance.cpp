#include "stdafx.h"
#include "VulkanInstance.h"

#include "GLFW/glfw3.h"
#include "../Input/Config.h"
#include "../Display/Display.h"

// Initilize static validation layers
std::vector<const char*> ym::VulkanInstance::validationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};

std::vector<const char*> ym::VulkanInstance::deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

ym::VulkanInstance::~VulkanInstance()
{
}

ym::VulkanInstance* ym::VulkanInstance::get()
{
	static VulkanInstance instance;
	return &instance;
}

void ym::VulkanInstance::init(std::vector<const char*> additionalDeviceExtensions, std::vector<const char*> additionalValidationLayers)
{
	this->enableValidationLayers = Config::get()->fetch<bool>("Debuglayer/active");
	createInstance();
	createSurface();
}

void ym::VulkanInstance::destroy()
{
	vkDestroySurfaceKHR(this->instance, this->surface, nullptr);

	if(this->enableValidationLayers)
		DestroyDebugUtilsMessengerEXT(this->instance, debugMessenger, nullptr);

	vkDestroyInstance(this->instance, nullptr);
}

ym::VulkanInstance::VulkanInstance()
{
	this->enableValidationLayers = false;
}

void ym::VulkanInstance::createInstance()
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "VulkanEngine";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "VulkanEngine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	// Fetch extensions
	std::vector<const char*> extensionNames = getRequiredExtensions({});

	// Check validation layer support
	if (!checkValidationLayerSupport(this->validationLayers)) {
		YM_ASSERT(false, "Failed to create validation layer!");
	}

	// Debug callback
	VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};

	// Define the instance.
	VkInstanceCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	info.pApplicationInfo = &appInfo;
	info.enabledExtensionCount = static_cast<uint32_t>(extensionNames.size());
	info.ppEnabledExtensionNames = extensionNames.data();
	if (this->enableValidationLayers)
	{
		info.enabledLayerCount = validationLayers.size();
		info.ppEnabledLayerNames = validationLayers.data();

		populateDebugMessengerCreateInfo(debugInfo);
		info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)& debugInfo;
	}
	else
	{
		info.enabledLayerCount = 0;
		info.ppEnabledLayerNames = nullptr;
		info.pNext = nullptr;
	}

	// Create the instance.
	VkResult result;
	result = vkCreateInstance(&info, nullptr, &this->instance);
	YM_ASSERT(result == VK_SUCCESS, "Failed to create instance!");

	// Create debug messenger.
	result = CreateDebugUtilsMessengerEXT(this->instance, &debugInfo, nullptr, &debugMessenger);
	YM_ASSERT(result == VK_SUCCESS, "Failed to set up debug messenger!");
}

std::vector<const char*> ym::VulkanInstance::getRequiredExtensions(std::vector<const char*> additionalExtensions)
{
	// Fetch standard extensions.
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	
	// Add debug extension
	if (this->enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	// Add additional extensions
	extensions.insert(extensions.end(), additionalExtensions.begin(), additionalExtensions.end());

	return extensions;
}

bool ym::VulkanInstance::checkValidationLayerSupport(const std::vector<const char*>& validationLayers)
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	// Go through all layer names and check if they are available for the instance.
	for (const char* layerName : validationLayers) {
		bool layerFound = false;
		// Go though each available layer and check if it matches with our validation layer name.
		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	// Only return true if all layers are supported.
	return true;
}

void ym::VulkanInstance::createSurface()
{
	VkResult result = glfwCreateWindowSurface(this->instance, Display::get()->getWindowPtr(), nullptr, &this->surface);
	YM_ASSERT(result == VK_SUCCESS, "Failed to create window surface!");
}

VKAPI_ATTR VkBool32 VKAPI_CALL ym::VulkanInstance::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	YM_LOG_ERROR("validation layer: {0}", pCallbackData->pMessage);
	return VK_FALSE;
}

VkResult ym::VulkanInstance::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	// Call the function vkCreateDebugUtilsMessengerEXT from the extension VK_EXT_DEBUG_UTILS_EXTENSION_NAME.
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void ym::VulkanInstance::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	// Call the function vkDestroyDebugUtilsMessengerEXT from the extension VK_EXT_DEBUG_UTILS_EXTENSION_NAME.
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

void ym::VulkanInstance::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugInfo)
{
	debugInfo = {};
	debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugInfo.pfnUserCallback = debugCallback;
	debugInfo.pUserData = nullptr; // Optional
}

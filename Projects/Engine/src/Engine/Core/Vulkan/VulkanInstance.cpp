#include "stdafx.h"
#include "VulkanInstance.h"

#include <set>

#include "GLFW/glfw3.h"
#include "../Input/Config.h"
#include "../Display/Display.h"
#include "DebugHelper.h"

// Initilize static validation layers
std::vector<const char*> ym::VulkanInstance::validationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};

std::vector<const char*> ym::VulkanInstance::deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
	//VK_NV_MESH_SHADER_EXTENSION_NAME
};

ym::VulkanInstance::~VulkanInstance()
{
}

ym::VulkanInstance* ym::VulkanInstance::get()
{
	static VulkanInstance instance;
	return &instance;
}

void ym::VulkanInstance::init()
{
	this->enableValidationLayers = Config::get()->fetch<bool>("Debuglayer/active");

	std::vector<const char*> additionalInstanceExtensions = {};
	createInstance(additionalInstanceExtensions);
	createSurface();
	pickPhysicalDevice();

	// Create the logical device.
	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.fillModeNonSolid = VK_TRUE;
	//deviceFeatures.logicOp = VK_TRUE;
	//deviceFeatures.pipelineStatisticsQuery = VK_TRUE;
	deviceFeatures.multiDrawIndirect = VK_TRUE;
	createLogicalDevice(deviceFeatures);
}

void ym::VulkanInstance::destroy()
{
	// Destroy the logical device.
	vkDestroyDevice(this->logicalDevice, nullptr);
	this->logicalDevice = VK_NULL_HANDLE;

	// Destroy the surface.
	vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
	this->surface = VK_NULL_HANDLE;

	// Destroy the debug messenger.
	if(this->enableValidationLayers)
		DestroyDebugUtilsMessengerEXT(this->instance, debugMessenger, nullptr);
	this->debugMessenger = VK_NULL_HANDLE;

	// Destroy the instance.
	vkDestroyInstance(this->instance, nullptr);
	this->instance = VK_NULL_HANDLE;
}

VkInstance ym::VulkanInstance::getInstance()
{
	return this->instance;
}

VkSurfaceKHR ym::VulkanInstance::getSurface()
{
	return this->surface;
}

VkPhysicalDevice ym::VulkanInstance::getPhysicalDevice()
{
	return this->physicalDevice;
}

VkDevice ym::VulkanInstance::getLogicalDevice()
{
	return this->logicalDevice;
}

ym::SwapChainSupportDetails ym::VulkanInstance::getSwapChainSupportDetails()
{
	return querySwapChainSupport(this->physicalDevice, this->surface);
}

ym::QueueFamilyIndices ym::VulkanInstance::getQueueFamilies()
{
	return findQueueFamilies(this->physicalDevice, this->surface);
}

ym::VulkanInstance::VulkanInstance()
{
	this->enableValidationLayers = false;
	this->instance = VK_NULL_HANDLE;
	this->debugMessenger = VK_NULL_HANDLE;
	this->surface = VK_NULL_HANDLE;
	this->physicalDevice = VK_NULL_HANDLE;
	this->logicalDevice = VK_NULL_HANDLE;
}

void ym::VulkanInstance::createInstance(std::vector<const char*> additionalInstanceExtensions)
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "VulkanEngine";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "VulkanEngine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	// Fetch extensions
	std::vector<const char*> extensionNames = getRequiredExtensions(additionalInstanceExtensions);

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
		info.enabledLayerCount = static_cast<uint32_t>(this->validationLayers.size());
		info.ppEnabledLayerNames = static_cast<const char* const*>(this->validationLayers.data());

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

std::vector<const char*> ym::VulkanInstance::getRequiredExtensions(std::vector<const char*> additionalInstanceExtensions)
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
	extensions.insert(extensions.end(), additionalInstanceExtensions.begin(), additionalInstanceExtensions.end());

	return extensions;
}

void ym::VulkanInstance::createSurface()
{
	VkResult result = glfwCreateWindowSurface(this->instance, Display::get()->getWindowPtr(), nullptr, &this->surface);
	YM_ASSERT(result == VK_SUCCESS, "Failed to create window surface!");
}

void ym::VulkanInstance::pickPhysicalDevice()
{
	// Get the number of connected physical devices.
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	YM_ASSERT(deviceCount != 0, "Failed to find GPUs with Vulkan support!");

	// Fetch all connected physical devices.
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	// Calculate the score for each physical device and choose the one with the highest score.
	int score = 0;
	for (const auto& device : devices) {
		int newScore = calculateDeviceSuitability(device);
		if (newScore > score) {
			physicalDevice = device;
			score = newScore;
		}
	}

	// Assert if no suitable physical device was found.
	YM_ASSERT(score != 0, "There is no suitable physical device!");
	YM_ASSERT(physicalDevice != VK_NULL_HANDLE, "Failed to find a suitable GPU!"); // This should never happen because of the previous assert.
}

int ym::VulkanInstance::calculateDeviceSuitability(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	// Exit if device is not suitable.
	if (!isDeviceSuitable(device, deviceFeatures))
		return 0;

	// Discrete GPUs have a significant performance advantage
	int score = 0;
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 1000;
	}

	// Maximum possible size of textures affects graphics quality
	score += deviceProperties.limits.maxImageDimension2D;

	YM_LOG_INFO("Using device: {0}", deviceProperties.deviceName);

	return score;
}

bool ym::VulkanInstance::isDeviceSuitable(VkPhysicalDevice device, VkPhysicalDeviceFeatures& deviceFeatures)
{
	QueueFamilyIndices index = findQueueFamilies(device, this->surface);
	bool extensionsSupported = checkDeviceExtensionSupport(device);
	bool swapChainAdequate = false;

	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, this->surface);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	// The device is suitable if it supports the queue families, extensions, swap chain extent and anisotropic filtering.
	return index.isComplete() && extensionsSupported && swapChainAdequate && deviceFeatures.samplerAnisotropy;
}

void ym::VulkanInstance::createLogicalDevice(VkPhysicalDeviceFeatures deviceFeatures)
{
	QueueFamilyIndices indices = findQueueFamilies(this->physicalDevice, this->surface);
	this->graphicsQueue.queueIndex = indices.graphicsFamily.value();
	this->presentQueue.queueIndex = indices.presentFamily.value();
	this->transferQueue.queueIndex = findQueueIndex(VK_QUEUE_TRANSFER_BIT, this->physicalDevice);
	this->computeQueue.queueIndex = findQueueIndex(VK_QUEUE_COMPUTE_BIT, this->physicalDevice);

	// Construct a set to hold the unique queue families (This will hold one queue family if both are the same).
	std::set<uint32_t> uniqueQueueFamilies = { this->graphicsQueue.queueIndex, this->presentQueue.queueIndex, 
		this->transferQueue.queueIndex, this->computeQueue.queueIndex };

	// Create the definition of the queue families which will be used.
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	float queuePriority = 1.0f;  // We want the highest priority for both the queues.
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}
	
	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(this->deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = this->deviceExtensions.data();

	// These are ignored by newer versions of vulkan. The logical device share validation layers with the instance.
	createInfo.enabledLayerCount = static_cast<uint32_t>(this->validationLayers.size());
	createInfo.ppEnabledLayerNames = this->validationLayers.data();


	// Create the logical device
	if (vkCreateDevice(this->physicalDevice, &createInfo, nullptr, &this->logicalDevice) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	// Fetch the queues.
	vkGetDeviceQueue(this->logicalDevice, this->graphicsQueue.queueIndex, 0, &this->graphicsQueue.queue);
	vkGetDeviceQueue(this->logicalDevice, this->presentQueue.queueIndex, 0, &this->presentQueue.queue);
	vkGetDeviceQueue(this->logicalDevice, this->transferQueue.queueIndex, 0, &this->transferQueue.queue);
	vkGetDeviceQueue(this->logicalDevice, this->computeQueue.queueIndex, 0, &this->computeQueue.queue);
}

bool ym::VulkanInstance::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	// Get the number of extensions.
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	// Fetch all extensions.
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	// Check if all the specified extensions are available.
	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
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

VKAPI_ATTR VkBool32 VKAPI_CALL ym::VulkanInstance::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	YM_LOG_ERROR("validation layer: {0}", pCallbackData->pMessage);
	/*YM_LOG_ERROR("cmdBufLabelCount: {0}", pCallbackData->cmdBufLabelCount);
	YM_LOG_ERROR("objectCount: {0}", pCallbackData->objectCount);
	for (uint32_t i = 0; i < pCallbackData->objectCount; i++)
	{
		YM_LOG_ERROR("   Object: {0:x} => {1}", pCallbackData->pObjects[i].objectHandle, NameBinder::getName(pCallbackData->pObjects[i].objectHandle).c_str());
	}
	//NameBinder::printContents();
	YM_LOG_ERROR("queueLabelCount: {0}", pCallbackData->queueLabelCount);*/
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

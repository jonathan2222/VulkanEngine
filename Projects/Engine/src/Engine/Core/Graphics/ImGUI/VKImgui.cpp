#include "stdafx.h"
#include "VKImgui.h"

#include "Engine/Core/Display/Display.h"
#include "Engine/Core/Vulkan/CommandBuffer.h"
#include "Engine/Core/Vulkan/CommandPool.h"
#include "Engine/Core/Vulkan/VulkanInstance.h"
#include "Engine/Core/Vulkan/Pipeline/RenderPass.h"
#include "Engine/Core/Vulkan/Buffers/Framebuffer.h"
#include "Engine/Core/Vulkan/SwapChain.h"
#include "Engine/Core/Input/Input.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "Utils/Imgui/imgui_impl_glfw.h"
#include "Utils/Imgui/imgui_impl_vulkan.h"

// TODO: When recreating swapChain make sure that this class works

ym::VKImgui::VKImgui()
	: window(nullptr), swapChain(nullptr), renderPass(nullptr), frameIndex(0),
	descPool(nullptr), commandPool(nullptr)
{
}

ym::VKImgui::~VKImgui()
{
}

void ym::VKImgui::init(SwapChain* swapChain)
{
	this->window = Display::get()->getWindowPtr();
	this->swapChain = swapChain;

	createCommandPoolAndBuffer();
	createDescriptorPool();
	createRenderPass();
	createFramebuffers();
	initImgui();
}

void ym::VKImgui::destroy()
{
	for (auto& buffer : this->framebuffers) {
		buffer->destroy();
		delete buffer;
	}
	this->renderPass->destroy();
	this->commandPool->destroy();

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	vkDestroyDescriptorPool(VulkanInstance::get()->getLogicalDevice(), this->descPool, nullptr);

	SAFE_DELETE(this->commandPool);
	SAFE_DELETE(this->renderPass);
}

void ym::VKImgui::begin(uint32_t frameIndex, float dt)
{
	this->frameIndex = frameIndex;

	auto& io = ImGui::GetIO();
	io.DeltaTime = dt;

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void ym::VKImgui::end()
{
	//if (Input::get()->isKeyToggled(GLFW_KEY_C))
	//	ImGui::SetMouseCursor(ImGuiMouseCursor_None);

	ImGui::EndFrame();
	ImGui::Render();
}

void ym::VKImgui::render()
{
	this->commandBuffers[this->frameIndex]->begin(0, nullptr);
	this->commandBuffers[this->frameIndex]->cmdBeginRenderPass(this->renderPass, this->framebuffers[this->frameIndex]->getFramebuffer(), this->swapChain->getExtent(), { {0.0f, 0.0f, 0.0f, 1.0f} }, VK_SUBPASS_CONTENTS_INLINE);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), this->commandBuffers[this->frameIndex]->getCommandBuffer());
	this->commandBuffers[this->frameIndex]->cmdEndRenderPass();
	this->commandBuffers[this->frameIndex]->end();
}

VkCommandBuffer ym::VKImgui::getCurrentCommandBuffer() const
{
	return this->commandBuffers[this->frameIndex]->getCommandBuffer();
}

void ym::VKImgui::createFramebuffers()
{
	this->framebuffers.resize(this->swapChain->getNumImages());

	for (size_t i = 0; i < this->swapChain->getNumImages(); i++)
	{
		this->framebuffers[i] = new Framebuffer;
		std::vector<VkImageView> imageViews;
		imageViews.push_back(this->swapChain->getImageViews()[i]);
		this->framebuffers[i]->init(this->swapChain->getNumImages(), this->renderPass, imageViews, this->swapChain->getExtent());
	}
}

void ym::VKImgui::createCommandPoolAndBuffer()
{
	this->commandPool = new CommandPool();
	this->commandPool->init(CommandPool::Queue::GRAPHICS, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	// Number of command buffers might differ in diferent implementations
	this->commandBuffers = this->commandPool->createCommandBuffers(this->swapChain->getNumImages(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}

void ym::VKImgui::createDescriptorPool()
{
	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
	poolInfo.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
	poolInfo.pPoolSizes = pool_sizes;
	VULKAN_CHECK(vkCreateDescriptorPool(VulkanInstance::get()->getLogicalDevice(), &poolInfo, nullptr, &this->descPool), "Failed to create ImGui descriptor pool!");
}

void ym::VKImgui::initImgui()
{
	VulkanInstance* instance = VulkanInstance::get();
	ImGui::CreateContext();

	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForVulkan(this->window, true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = instance->getInstance();
	init_info.PhysicalDevice = instance->getPhysicalDevice();
	init_info.Device = instance->getLogicalDevice();
	init_info.QueueFamily = VK_QUEUE_GRAPHICS_BIT;
	init_info.Queue = instance->getGraphicsQueue().queue;
	init_info.PipelineCache = VK_NULL_HANDLE;
	init_info.DescriptorPool = this->descPool;
	init_info.Allocator = nullptr;
	init_info.MinImageCount = this->swapChain->getNumImages();
	init_info.ImageCount = this->swapChain->getNumImages();
	init_info.CheckVkResultFn = nullptr; // Change this to ImGui:s implementation of error check to get errors
	ImGui_ImplVulkan_Init(&init_info, this->renderPass->getRenderPass());

	// Upload fonts to the GPU using command buffers
	CommandBuffer* b = this->commandPool->beginSingleTimeCommand();
	ImGui_ImplVulkan_CreateFontsTexture(b->getCommandBuffer());
	this->commandPool->endSingleTimeCommand(b);
}

void ym::VKImgui::createRenderPass()
{
	this->renderPass = new RenderPass();

	VkAttachmentDescription attachment = {};
	attachment.format = this->swapChain->getImageFormat();
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // LOAD_OP_LOAD instead of clear to be able to draw on top of the program
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // InitalLayout on color attachment since we are drawing on top
	attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // This render pass should be the last, hence present layout
	this->renderPass->addColorAttachment(attachment);

	RenderPass::SubpassInfo subpassInfo;
	subpassInfo.colorAttachmentIndices = { 0 };
	this->renderPass->addSubpass(subpassInfo);

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;  // or VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // This is the different part compared to the default
	this->renderPass->addSubpassDependency(dependency);

	this->renderPass->init();
}

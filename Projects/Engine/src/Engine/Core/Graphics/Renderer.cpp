#include "stdafx.h"
#include "Renderer.h"

#include "../Input/Config.h"
#include "Engine/Core/Vulkan/Factory.h"
#include "Engine/Core/Scene/GLTFLoader.h"
#include "Engine/Core/Application/LayerManager.h"
#include "Engine/Core/Threading/ThreadManager.h"
#include "Engine/Core/Vulkan/Pipeline/DescriptorSet.h"
#include "Engine/Core/Scene/ObjectManager.h"
#include "Engine/Core/Scene/GameObject.h"

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

	initInheritenceData();

	// Each renderer has a corresponding thread.
	this->modelRenderer.init(&this->swapChain, (uint32_t)ERendererType::RENDER_TYPE_MODEL, &this->renderPass, &this->renderInheritanceData); // Uses thread 0.

	this->cubeMapRenderer.init(&this->swapChain, (uint32_t)ERendererType::RENDER_TYPE_CUBE_MAP, &this->renderPass, &this->renderInheritanceData); // Uses thread 1

	//this->terrainRenderer.init(&this->swapChain, (uint32_t)ERendererType::RENDER_TYPE_TERRAIN, &this->renderPass, &this->sceneDescriptors); // Uses thread 2. (Will not work, 2 is occupied by GLTFLoader)

	GLTFLoader::init();

	createSyncObjects();

	CommandPool& graphicsPool = LayerManager::get()->getCommandPools()->graphicsPool;
	this->primaryCommandBuffersGraphics = graphicsPool.createCommandBuffers(this->swapChain.getNumImages(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	CommandPool& computePool = LayerManager::get()->getCommandPools()->computePool;
	//this->primaryCommandBuffersCompute = computePool.createCommandBuffers(this->swapChain.getNumImages(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}

void ym::Renderer::preDestroy()
{
	vkDeviceWaitIdle(VulkanInstance::get()->getLogicalDevice());
	ThreadManager::destroy();
}

void ym::Renderer::destroy()
{
	GLTFLoader::destroy();

	// Destroy sub-renderers.
	this->modelRenderer.destroy();
	this->cubeMapRenderer.destroy();
	//this->terrainRenderer.destroy();

	// Destroy scene data.
	for (UniformBuffer& ubo : this->sceneUBOs)
		ubo.destroy();
	this->descriptorPool.destroy();

	destroyInheritanceData();

	// Destroy the rest.
	this->renderPass.destroy();
	this->depthTexture->destroy();
	SAFE_DELETE(this->depthTexture);
	for (Framebuffer& framebuffer : this->framebuffers) framebuffer.destroy();
	destroySyncObjects();
	this->swapChain.destory();
}

void ym::Renderer::setActiveCamera(Camera* camera)
{
	this->activeCamera = camera;
	//this->terrainRenderer.setActiveCamera(camera);
}

bool ym::Renderer::begin()
{
	// Check if models have loaded.
	GLTFLoader::update();

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

	this->modelRenderer.begin(this->imageIndex, this->inheritanceInfo);
	this->cubeMapRenderer.begin(this->imageIndex, this->inheritanceInfo);
	//this->terrainRenderer.begin(this->imageIndex, this->inheritanceInfo);

	return true;
}

void ym::Renderer::drawModel(Model* model)
{
	glm::mat4 transform(1.0f);
	this->modelRenderer.drawModel(this->imageIndex, model, transform);
}

void ym::Renderer::drawModel(Model* model, const glm::mat4& transform)
{
	this->modelRenderer.drawModel(this->imageIndex, model, transform);
}

void ym::Renderer::drawModel(Model* model, const std::vector<glm::mat4>& transforms)
{
	this->modelRenderer.drawModel(this->imageIndex, model, transforms);
}

void ym::Renderer::drawAllModels(ObjectManager* objectManager)
{
	auto& gameObjects = objectManager->getGameObjects();
	for (auto& batch : gameObjects)
	{
		auto& objs = batch.second;
		uint32_t size = (uint32_t)objs.size();
		if (size > 0)
		{
			Model* model = objs[0]->getModel();
			std::vector<glm::mat4> transforms(objs.size());
			for (uint32_t i = 0; i < size; i++)
				transforms[i] = objs[i]->getTransform();
			drawModel(model, transforms);
		}
	}
}

void ym::Renderer::drawCubeMap(CubeMap* cubeMap, const glm::mat4& transform)
{
	this->cubeMapRenderer.drawCubeMap(this->imageIndex, cubeMap, transform);
}

void ym::Renderer::drawCubeMap(CubeMap* cubeMap, const std::vector<glm::mat4>& transforms)
{
	this->cubeMapRenderer.drawCubeMap(this->imageIndex, cubeMap, transforms);
}

void ym::Renderer::drawTerrain(Terrain* terrain, const glm::mat4& transform)
{
	YM_LOG_WARN("drawTerrain() is not implemented!");
	//this->terrainRenderer.drawTerrain(this->imageIndex, terrain, transform);
}

bool ym::Renderer::end()
{
	// Update camera.
	SceneData sceneData;
	sceneData.proj = this->activeCamera->getProjection();
	sceneData.view = this->activeCamera->getView();
	sceneData.cPos = this->activeCamera->getPosition();
	this->sceneUBOs[imageIndex].transfer(&sceneData, sizeof(SceneData), 0);

	// End renderers.
	this->modelRenderer.end(this->imageIndex);
	this->cubeMapRenderer.end(this->imageIndex);
	//this->terrainRenderer.end(this->imageIndex);

	// Submit all work.
	submit();

	// Present new frame.
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
	this->framesInFlight = this->swapChain.getNumImages()-1;
	this->imageAvailableSemaphores.resize(this->framesInFlight);
	this->renderFinishedSemaphores.resize(this->framesInFlight);
	//this->computeSemaphores.resize(this->framesInFlight);
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
		//VULKAN_CHECK(vkCreateSemaphore(VulkanInstance::get()->getLogicalDevice(), &SemaCreateInfo, nullptr, &this->computeSemaphores[i]), "Failed to create compute semaphores");
		VULKAN_CHECK(vkCreateFence(VulkanInstance::get()->getLogicalDevice(), &fenceCreateInfo, nullptr, &this->inFlightFences[i]), "Failed to create in flight fences");
	}
}

void ym::Renderer::destroySyncObjects()
{
	for (uint32_t i = 0; i < this->framesInFlight; i++) {
		vkDestroySemaphore(VulkanInstance::get()->getLogicalDevice(), this->imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(VulkanInstance::get()->getLogicalDevice(), this->renderFinishedSemaphores[i], nullptr);
		//vkDestroySemaphore(VulkanInstance::get()->getLogicalDevice(), this->computeSemaphores[i], nullptr);
		vkDestroyFence(VulkanInstance::get()->getLogicalDevice(), this->inFlightFences[i], nullptr);
	}
}

void ym::Renderer::submit()
{
	VulkanInstance* instance = VulkanInstance::get();
	// Compute
	/*{
		// Gather all secondary buffers.
		std::vector<CommandBuffer*>& secondaryBuffers = this->terrainRenderer.getComputeBuffers();
		std::vector<VkCommandBuffer> vkCommands;
		vkCommands.push_back(secondaryBuffers[this->imageIndex]->getCommandBuffer());

		CommandBuffer* buffer = this->primaryCommandBuffersCompute[this->imageIndex];
		{
			buffer->begin(0, nullptr);

			this->terrainRenderer.preRecordCompute(this->imageIndex, buffer);
			buffer->cmdExecuteCommands((uint32_t)vkCommands.size(), vkCommands.data());
			this->terrainRenderer.postRecordCompute(this->imageIndex, buffer);

			buffer->end();
		}

		// Submit commands to GPU.
		VkSubmitInfo computeSubmitInfo = { };
		computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		std::vector<VkPipelineStageFlags> waitStages;
		std::vector<VkSemaphore> waitSemaphores;
		computeSubmitInfo.waitSemaphoreCount = waitSemaphores.size();
		computeSubmitInfo.pWaitSemaphores = waitSemaphores.data();
		computeSubmitInfo.pWaitDstStageMask = waitStages.data();

		std::array<VkCommandBuffer, 1> buff = { buffer->getCommandBuffer() };
		computeSubmitInfo.commandBufferCount = buff.size();
		computeSubmitInfo.pCommandBuffers = buff.data();
		computeSubmitInfo.signalSemaphoreCount = 1;
		computeSubmitInfo.pSignalSemaphores = &this->computeSemaphores[this->currentFrame];

		VulkanQueue& computeQueue = instance->getComputeQueue();
		VULKAN_CHECK(vkQueueSubmit(computeQueue.queue, 1, &computeSubmitInfo, VK_NULL_HANDLE), "Failed to submit compute queue!");
	}*/

	// Graphics
	{
		// Gather all secondary buffers.
		std::vector<VkCommandBuffer> vkCommands;

		// Fetch secondary buffers from the ModelRenderer.
		std::vector<CommandBuffer*>& secondaryBuffersModel = this->modelRenderer.getBuffers();
		vkCommands.push_back(secondaryBuffersModel[this->imageIndex]->getCommandBuffer());

		// Fetch secondary buffers from the CubeMapRenderer.
		std::vector<CommandBuffer*>& secondaryBuffersCubeMap = this->cubeMapRenderer.getBuffers();
		vkCommands.push_back(secondaryBuffersCubeMap[this->imageIndex]->getCommandBuffer());

		// Record and execute them on the current primary command buffer.
		CommandBuffer* buffer = this->primaryCommandBuffersGraphics[this->imageIndex];
		{
			buffer->begin(0, nullptr);

			//this->terrainRenderer.preRecordGraphics(this->imageIndex, buffer);

			std::vector<VkClearValue> clearValues = {};
			VkClearValue value;
			value.color = { 0.0f, 0.0f, 0.0f, 1.0f };
			clearValues.push_back(value);
			value.depthStencil = { 1.0f, 0 };
			clearValues.push_back(value);
			buffer->cmdBeginRenderPass(&this->renderPass, this->framebuffers[this->imageIndex].getFramebuffer(), this->swapChain.getExtent(), clearValues, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
			buffer->cmdExecuteCommands((uint32_t)vkCommands.size(), vkCommands.data());
			buffer->cmdEndRenderPass();

			//this->terrainRenderer.postRecordGraphics(this->imageIndex, buffer);

			buffer->end();
		}

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

		VulkanQueue& graphicsQueue = instance->getGraphicsQueue();
		vkResetFences(instance->getLogicalDevice(), 1, &this->inFlightFences[this->currentFrame]);
		VULKAN_CHECK(vkQueueSubmit(graphicsQueue.queue, 1, &submitInfo, this->inFlightFences[this->currentFrame]), "Failed to sumbit commandbuffer!");
	}
}

void ym::Renderer::initInheritenceData()
{
	setupSceneDescriptors();

	// Create separate command buffers for each thread.
	this->renderInheritanceData.threadData.resize(ERendererType::RENDER_TYPE_SIZE);
	for (uint32_t i = 0; i < this->renderInheritanceData.threadData.size(); i++)
	{
		ThreadData& td = this->renderInheritanceData.threadData[i];
		td.init(i, &this->swapChain);
	}
}

void ym::Renderer::destroyInheritanceData()
{
	this->renderInheritanceData.sceneDescriptors.layout.destroy();
	// Destory thread dependent data
	for (ThreadData& td : this->renderInheritanceData.threadData)
		td.destory();
}

void ym::Renderer::setupSceneDescriptors()
{
	this->sceneUBOs.resize(this->swapChain.getNumImages());
	for (auto& ubo : this->sceneUBOs)
		ubo.init(sizeof(SceneData));

	this->renderInheritanceData.sceneDescriptors.layout.add(new UBO(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)); // Camera
	this->renderInheritanceData.sceneDescriptors.layout.init();

	// There is only one scene at a time => one descriptor set.
	this->descriptorPool.addDescriptorLayout(this->renderInheritanceData.sceneDescriptors.layout, 1);

	// The scene will be used in all frames => make copies of them.
	uint32_t numFrames = this->swapChain.getNumImages();
	descriptorPool.init(numFrames);

	this->renderInheritanceData.sceneDescriptors.sets.resize(this->swapChain.getNumImages(), VK_NULL_HANDLE);
	for (uint32_t i = 0; i < this->swapChain.getNumImages(); i++)
	{
		DescriptorSet sceneSet;
		sceneSet.init(this->renderInheritanceData.sceneDescriptors.layout, &this->renderInheritanceData.sceneDescriptors.sets[i], &this->descriptorPool);
		sceneSet.setBufferDesc(0, this->sceneUBOs[i].getDescriptor());
		sceneSet.update();
	}
}

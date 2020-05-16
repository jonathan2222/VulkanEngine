#include "stdafx.h"
#include "CubeMapRenderer.h"

#include "Engine/Core/Threading/ThreadManager.h"
#include "Engine/Core/Scene/Vertex.h"
#include "Engine/Core/Vulkan/Pipeline/DescriptorSet.h"
#include "Engine/Core/Scene/Model/Material.h"
#include "Engine/Core/Vulkan/Pipeline/RenderPass.h"
#include "Engine/Core/Application/LayerManager.h"
#include "Engine/Core/Vulkan/Factory.h"
#include "Engine/Core/Vulkan/VulkanHelperfunctions.h"
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/matrix_transform.hpp>

ym::CubeMapRenderer::CubeMapRenderer()
{
}

ym::CubeMapRenderer::~CubeMapRenderer()
{
}

void ym::CubeMapRenderer::init(SwapChain* swapChain, uint32_t threadID, RenderPass* renderPass, RenderInheritanceData* renderInheritanceData)
{
	this->swapChain = swapChain;
	this->threadID = threadID;
	this->commandPool.init(CommandPool::Queue::GRAPHICS, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	this->commandBuffers = this->commandPool.createCommandBuffers(this->swapChain->getNumImages(), VK_COMMAND_BUFFER_LEVEL_SECONDARY);

	this->shader.addStage(Shader::Type::VERTEX, YM_ASSETS_FILE_PATH + "Shaders/skyboxVert.spv");
	this->shader.addStage(Shader::Type::FRAGMENT, YM_ASSETS_FILE_PATH + "Shaders/skyboxFrag.spv");
	this->shader.init();

	this->shouldRecreateDescriptors.resize(this->swapChain->getNumImages(), false);
	this->descriptorPools.resize(this->swapChain->getNumImages());
	this->renderInheritanceData = renderInheritanceData;

	createDescriptorLayouts();
	std::vector<DescriptorLayout> descriptorLayouts = { this->renderInheritanceData->sceneDescriptors.layout, descriptorSetLayouts.material, descriptorSetLayouts.model };
	VkVertexInputBindingDescription vertexBindingDescription = {};
	vertexBindingDescription.binding = 0;
	vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	vertexBindingDescription.stride = sizeof(glm::vec4);
	VkVertexInputAttributeDescription vertexAttributeDescription = {};
	vertexAttributeDescription.binding = 0;
	vertexAttributeDescription.location = 0;
	vertexAttributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	vertexAttributeDescription.offset = 0;
	PipelineInfo pipelineInfo = {};
	pipelineInfo.vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	pipelineInfo.vertexInputInfo.vertexBindingDescriptionCount = 1;
	pipelineInfo.vertexInputInfo.vertexAttributeDescriptionCount = 1;
	pipelineInfo.vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDescription;
	pipelineInfo.vertexInputInfo.pVertexAttributeDescriptions = &vertexAttributeDescription;

	pipelineInfo.rasterizer = {};
	pipelineInfo.rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	pipelineInfo.rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	pipelineInfo.rasterizer.lineWidth = 1.0f;
	pipelineInfo.rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
	pipelineInfo.rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	this->pipeline.setPipelineInfo(PipelineInfoFlag::RASTERIZATION | PipelineInfoFlag::VERTEX_INPUT, pipelineInfo);
	this->pipeline.setDescriptorLayouts(descriptorLayouts);
	this->pipeline.setGraphicsPipelineInfo(swapChain->getExtent(), renderPass);
	this->pipeline.init(Pipeline::Type::GRAPHICS, &this->shader);
}

void ym::CubeMapRenderer::destroy()
{
	this->shader.destroy();
	this->pipeline.destroy();
	this->commandPool.destroy();
	for (DescriptorPool& pool : this->descriptorPools)
		pool.destroy();

	this->descriptorSetLayouts.model.destroy();
	this->descriptorSetLayouts.material.destroy();

	for (auto& batch : this->drawBatch)
		for (auto& drawData : batch)
			drawData.second.transformsBuffer.destroy();
}

ym::Texture* ym::CubeMapRenderer::convertEquirectangularToCubemap(uint32_t sideSize, Texture* texture, CubeMap* cubeMap)
{
	VkExtent2D extent = {};
	extent.width = sideSize;
	extent.height = sideSize;

	TextureDesc textureDesc;
	textureDesc.width = extent.width;
	textureDesc.height = extent.height;
	textureDesc.format = texture->textureDesc.format;
	textureDesc.data = nullptr;
	Texture* newTexture = Factory::createCubeMapTexture(textureDesc, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_QUEUE_GRAPHICS_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

	textureDesc = {};
	textureDesc.width = sideSize;
	textureDesc.height = sideSize;
	textureDesc.format = texture->textureDesc.format;
	textureDesc.data = nullptr;
	Texture* image = Factory::createTexture(textureDesc, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_QUEUE_GRAPHICS_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

	CommandPool* commandPool = &LayerManager::get()->getCommandPools()->graphicsPool;
	CommandBuffer* commandBuffer = commandPool->beginSingleTimeCommand();
	image->image.setLayout(
		commandBuffer,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED, 
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	commandPool->endSingleTimeCommand(commandBuffer);

	/*
	Image::TransistionDesc imgTransistionDesc;
	imgTransistionDesc.pool = &LayerManager::get()->getCommandPools()->graphicsPool;
	imgTransistionDesc.layerCount = 1;
	imgTransistionDesc.format = image->textureDesc.format;
	imgTransistionDesc.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imgTransistionDesc.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	image->image.transistionLayout(imgTransistionDesc);
	*/

	Pipeline pipeline;
	Shader shader;
	shader.addStage(Shader::Type::VERTEX, YM_ASSETS_FILE_PATH + "Shaders/cubeVert.spv");
	shader.addStage(Shader::Type::FRAGMENT, YM_ASSETS_FILE_PATH + "Shaders/cubeConvertFrag.spv");
	shader.init();

	DescriptorPool descPool;
	DescriptorLayout descLayout;
	descLayout.add(new IMG(VK_SHADER_STAGE_FRAGMENT_BIT)); // Equirectangular texture
	descLayout.init();

	descPool.addDescriptorLayout(descLayout, 1);
	descPool.init(1);

	VkDescriptorSet descriptorSet;
	DescriptorSet modelSet;
	modelSet.init(descLayout, &descriptorSet, &descPool);
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = texture->image.getLayout();
	imageInfo.imageView = texture->imageView.getImageView();
	imageInfo.sampler = cubeMap->getSampler()->getSampler();
	modelSet.setImageDesc(0, imageInfo);
	modelSet.update();

	RenderPass renderPass;
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = texture->textureDesc.format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	renderPass.addColorAttachment(colorAttachment);

	// Use subpass dependencies for layout transitions
	VkSubpassDependency dependency1 = {};
	dependency1.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency1.dstSubpass = 0;
	dependency1.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependency1.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency1.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependency1.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency1.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	renderPass.addSubpassDependency(dependency1);
	VkSubpassDependency dependency2 = {};
	dependency2.srcSubpass = 0;
	dependency2.dstSubpass = VK_SUBPASS_EXTERNAL;
	dependency2.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency2.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependency2.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency2.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependency2.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	renderPass.addSubpassDependency(dependency2);

	RenderPass::SubpassInfo subpassInfo;
	subpassInfo.colorAttachmentIndices = {0};
	renderPass.addSubpass(subpassInfo);
	renderPass.init();

	std::vector<DescriptorLayout> descriptorLayouts = { descLayout };
	VkVertexInputBindingDescription vertexBindingDescription = {};
	vertexBindingDescription.binding = 0;
	vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	vertexBindingDescription.stride = sizeof(glm::vec4);
	VkVertexInputAttributeDescription vertexAttributeDescription = {};
	vertexAttributeDescription.binding = 0;
	vertexAttributeDescription.location = 0;
	vertexAttributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	vertexAttributeDescription.offset = 0;
	PipelineInfo pipelineInfo = {};
	pipelineInfo.vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	pipelineInfo.vertexInputInfo.vertexBindingDescriptionCount = 1;
	pipelineInfo.vertexInputInfo.vertexAttributeDescriptionCount = 1;
	pipelineInfo.vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDescription;
	pipelineInfo.vertexInputInfo.pVertexAttributeDescriptions = &vertexAttributeDescription;

	pipelineInfo.rasterizer = {};
	pipelineInfo.rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	pipelineInfo.rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	pipelineInfo.rasterizer.lineWidth = 1.0f;
	pipelineInfo.rasterizer.cullMode = VK_CULL_MODE_NONE;
	pipelineInfo.rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	pipelineInfo.depthStencil = {};
	pipelineInfo.depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	pipelineInfo.depthStencil.depthTestEnable = VK_FALSE;
	pipelineInfo.depthStencil.depthWriteEnable = VK_FALSE;
	pipelineInfo.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	pipelineInfo.depthStencil.depthBoundsTestEnable = VK_FALSE;
	pipelineInfo.depthStencil.minDepthBounds = 0.0f; // Optional
	pipelineInfo.depthStencil.maxDepthBounds = 1.0f; // Optional
	pipelineInfo.depthStencil.stencilTestEnable = VK_FALSE;
	pipelineInfo.depthStencil.front = {}; // Optional
	pipelineInfo.depthStencil.back = {}; // Optional

	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.size = sizeof(glm::mat4);
	pushConstantRange.offset = 0;
	pipeline.setPushConstant(pushConstantRange);
	pipeline.setPipelineInfo(PipelineInfoFlag::DEAPTH_STENCIL| PipelineInfoFlag::RASTERIZATION | PipelineInfoFlag::VERTEX_INPUT, pipelineInfo);
	pipeline.setDescriptorLayouts(descriptorLayouts);
	pipeline.setGraphicsPipelineInfo(extent, &renderPass);
	pipeline.init(Pipeline::Type::GRAPHICS, &shader);

	std::vector<VkClearValue> clearValues = {};
	VkClearValue value;
	value.color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues.push_back(value);

	VkFramebuffer frameBuffer;
	VkFramebufferCreateInfo frameBufferInfo = {};
	frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferInfo.renderPass = renderPass.getRenderPass();
	frameBufferInfo.width = extent.width;
	frameBufferInfo.height = extent.height;
	frameBufferInfo.attachmentCount = 1;
	VkImageView imageView = image->imageView.getImageView();
	frameBufferInfo.pAttachments = &imageView;
	frameBufferInfo.layers = 1;
	vkCreateFramebuffer(VulkanInstance::get()->getLogicalDevice(), &frameBufferInfo, nullptr, &frameBuffer);
	
	std::vector<glm::mat4> matrices = {
		// POSITIVE_X
		glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		// NEGATIVE_X
		glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		// POSITIVE_Y
		glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		// NEGATIVE_Y
		glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		// POSITIVE_Z
		glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		// NEGATIVE_Z
		glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
	};

	commandBuffer = commandPool->beginSingleTimeCommand();

	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1;
	subresourceRange.layerCount = 6;
	newTexture->image.setLayout(
		commandBuffer,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		subresourceRange);

	for (uint32_t f = 0; f < 6; f++) {
		commandBuffer->cmdBeginRenderPass(&renderPass, frameBuffer, extent, clearValues, VK_SUBPASS_CONTENTS_INLINE);

		VkBuffer buffer = cubeMap->getVertexBuffer()->getBuffer();

		// Update shader push constant block
		glm::mat4 mvp = glm::perspective((float)(3.1415f / 2.0), 1.0f, 0.1f, (float)sideSize) * matrices[f];
		vkCmdPushConstants(commandBuffer->getCommandBuffer(), pipeline.getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &mvp);

		commandBuffer->cmdBindPipeline(&pipeline);
		VkDeviceSize offset = 0;
		commandBuffer->cmdBindVertexBuffers(0, 1, &buffer, &offset);

		std::vector<VkDescriptorSet> sets = { descriptorSet };
		std::vector<uint32_t> offsets;
		commandBuffer->cmdBindDescriptorSets(&pipeline, 0, sets, offsets);
		commandBuffer->cmdDraw(36, 1, 0, 0);

		commandBuffer->cmdEndRenderPass();

		image->image.setLayout(
			commandBuffer,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		// Copy region for transfer from framebuffer to cube face
		VkImageCopy copyRegion = {};
		copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.srcSubresource.baseArrayLayer = 0;
		copyRegion.srcSubresource.mipLevel = 0;
		copyRegion.srcSubresource.layerCount = 1;
		copyRegion.srcOffset = { 0, 0, 0 };
		copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.dstSubresource.baseArrayLayer = f;
		copyRegion.dstSubresource.mipLevel = 0;
		copyRegion.dstSubresource.layerCount = 1;
		copyRegion.dstOffset = { 0, 0, 0 };
		copyRegion.extent.width = extent.width;
		copyRegion.extent.height = extent.height;
		copyRegion.extent.depth = 1;

		vkCmdCopyImage(
			commandBuffer->getCommandBuffer(),
			image->image.getImage(),
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			newTexture->image.getImage(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&copyRegion);

		// Transform framebuffer color attachment back (Used if more mipmaps else not necessary!)
		image->image.setLayout(
			commandBuffer,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}

	newTexture->image.setLayout(
		commandBuffer,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		subresourceRange);

	commandPool->endSingleTimeCommand(commandBuffer);

	vkDestroyFramebuffer(VulkanInstance::get()->getLogicalDevice(), frameBuffer, nullptr);
	descLayout.destroy();
	descPool.destroy();
	renderPass.destroy();
	shader.destroy();
	pipeline.destroy();

	image->destroy();
	SAFE_DELETE(image);

	return newTexture;
}

void ym::CubeMapRenderer::begin(uint32_t imageIndex, VkCommandBufferInheritanceInfo inheritanceInfo)
{
	this->inheritanceInfo = inheritanceInfo;

	if (this->drawBatch.empty() == false)
	{
		for (auto& drawData : this->drawBatch[imageIndex])
			drawData.second.exists = false;
	}

	// Clear draw data if descriptor pool should be recreated.
	if (this->shouldRecreateDescriptors[imageIndex])
	{
		for (auto& drawData : this->drawBatch[imageIndex])
			drawData.second.transformsBuffer.destroy();
		this->drawBatch[imageIndex].clear();
	}
}

void ym::CubeMapRenderer::drawCubeMap(uint32_t imageIndex, CubeMap* model, const glm::mat4& transform)
{
	DrawData drawData = {};
	drawData.model = model;
	drawData.transforms.push_back(transform);
	addModelToBatch(imageIndex, drawData);
}

void ym::CubeMapRenderer::drawCubeMap(uint32_t imageIndex, CubeMap* model, const std::vector<glm::mat4>& transforms)
{
	DrawData drawData = {};
	drawData.model = model;
	drawData.transforms.insert(drawData.transforms.begin(), transforms.begin(), transforms.end());
	addModelToBatch(imageIndex, drawData);
}

void ym::CubeMapRenderer::end(uint32_t imageIndex)
{
	if (this->shouldRecreateDescriptors[imageIndex])
	{
		// Fetch number of nodes and materials.
		uint32_t modelCount = 0;
		uint32_t materialCount = 0;
		for (auto& drawData : this->drawBatch[imageIndex])
		{
			modelCount++; // One mesh
			materialCount++; // One material
		}
		recreateDescriptorPool(imageIndex, materialCount, modelCount);

		// Recreate descriptor sets
		createDescriptorsSets(imageIndex, this->drawBatch[imageIndex]);

		this->shouldRecreateDescriptors[imageIndex] = false;
	}

	// Start recording all draw commands on the thread.
	CommandBuffer* currentBuffer = this->commandBuffers[imageIndex];
	currentBuffer->begin(VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, &inheritanceInfo);
	if (this->drawBatch.empty() == false)
	{
		for (auto& drawData : this->drawBatch[imageIndex])
		{
			if (drawData.second.exists)
			{
				uint32_t instanceCount = (uint32_t)drawData.second.transforms.size();
				drawData.second.transformsBuffer.transfer(drawData.second.transforms.data(), sizeof(glm::mat4) * instanceCount, 0);
				ThreadManager::addWork(this->threadID, [=]() {
					recordCubeMap(drawData.second.model, imageIndex, drawData.second.descriptorSet, instanceCount, currentBuffer);
					});
			}
		}

		ThreadManager::wait(this->threadID);
	};
	currentBuffer->end();
}

std::vector<ym::CommandBuffer*>& ym::CubeMapRenderer::getBuffers()
{
	return this->commandBuffers;
}

void ym::CubeMapRenderer::recordCubeMap(CubeMap* model, uint32_t imageIndex, VkDescriptorSet instanceDescriptorSet, uint32_t instanceCount, CommandBuffer* cmdBuffer)
{
	VkBuffer buffer = model->getVertexBuffer()->getBuffer();
	if (buffer == VK_NULL_HANDLE)
		return;

	cmdBuffer->cmdBindPipeline(&this->pipeline);
	VkDeviceSize offset = 0;
	cmdBuffer->cmdBindVertexBuffers(0, 1, &buffer, &offset);

	std::vector<VkDescriptorSet> sets = {
		this->renderInheritanceData->sceneDescriptors.sets[imageIndex],
		model->descriptorSet,
		instanceDescriptorSet
	};
	std::vector<uint32_t> offsets;
	cmdBuffer->cmdBindDescriptorSets(&this->pipeline, 0, sets, offsets);
	cmdBuffer->cmdDraw(36, instanceCount, 0, 0);
}

void ym::CubeMapRenderer::createDescriptorLayouts()
{
	// Model
	descriptorSetLayouts.model.add(new SSBO(VK_SHADER_STAGE_VERTEX_BIT)); // Instance transformations.
	descriptorSetLayouts.model.init();

	// Material
	descriptorSetLayouts.material.add(new IMG(VK_SHADER_STAGE_FRAGMENT_BIT)); // CubeMap
	descriptorSetLayouts.material.init();
}

void ym::CubeMapRenderer::recreateDescriptorPool(uint32_t imageIndex, uint32_t materialCount, uint32_t modelCount)
{
	// Destroy the descriptor pool and its descriptor sets if it should be recreated.
	DescriptorPool& descriptorPool = this->descriptorPools[imageIndex];
	if (descriptorPool.wasCreated())
		descriptorPool.destroy();

	// There are many models with its own data => own descriptor set, multiply with the number of models.
	descriptorPool.addDescriptorLayout(descriptorSetLayouts.model, modelCount);
	// Same for the material but multiply instead with the number of materials.
	descriptorPool.addDescriptorLayout(descriptorSetLayouts.material, materialCount);

	descriptorPool.init(1);
}

void ym::CubeMapRenderer::createDescriptorsSets(uint32_t imageIndex, std::map<uint64_t, DrawData>& drawBatch)
{
	VkDevice device = VulkanInstance::get()->getLogicalDevice();

	// Nodes, Materials and Models
	for (auto& drawData : drawBatch)
	{
		// Models
		{
			DescriptorSet modelSet;
			modelSet.init(descriptorSetLayouts.model, &drawData.second.descriptorSet, &this->descriptorPools[imageIndex]);
			modelSet.setBufferDesc(0, drawData.second.transformsBuffer.getDescriptor());
			modelSet.update();
		}

		// Materials
		CubeMap* cubeMap = drawData.second.model;
		{
			// Create descriptor sets for each material.
			Material::Tex texture = {};
			texture.texture = drawData.second.model->getTexture();
			texture.sampler = drawData.second.model->getSampler();

			VkDescriptorImageInfo imageInfo;
			imageInfo.imageLayout = texture.texture->image.getLayout();
			imageInfo.imageView = texture.texture->imageView.getImageView();
			imageInfo.sampler = texture.sampler->getSampler();

			{
				DescriptorSet materialSet;
				materialSet.init(descriptorSetLayouts.material, &cubeMap->descriptorSet, &this->descriptorPools[imageIndex]);
				materialSet.setImageDesc(0, imageInfo);
				materialSet.update();
			}
		}
	}
}

uint64_t ym::CubeMapRenderer::getUniqueId(uint32_t modelId, uint32_t instanceCount)
{
	uint64_t instanceCount64 = static_cast<uint64_t>(instanceCount) & 0xFFFFFFFF;
	uint64_t modelId64 = static_cast<uint64_t>(modelId) & 0xFFFFFFFF;
	uint64_t id = (modelId64 << 32) | instanceCount64;
	return id;
}

void ym::CubeMapRenderer::addModelToBatch(uint32_t imageIndex, DrawData& drawData)
{
	if (this->drawBatch.empty())
		this->drawBatch.resize(this->swapChain->getNumImages());

	uint32_t instanceCount = (uint32_t)drawData.transforms.size();
	uint64_t id = getUniqueId(drawData.model->getUniqueId(), instanceCount);
	auto& it = this->drawBatch[imageIndex].find(id);
	if (it == this->drawBatch[imageIndex].end())
	{
		// Create buffer for transformations.
		drawData.transformsBuffer.init(sizeof(glm::mat4) * (uint64_t)instanceCount);
		this->drawBatch[imageIndex][id] = drawData;
		this->drawBatch[imageIndex][id].descriptorSet = VK_NULL_HANDLE;
		this->drawBatch[imageIndex][id].exists = true;

		// Tell renderer to recreate descriptors before rendering.
		this->shouldRecreateDescriptors[imageIndex] = true;
	}
	else
	{
		it->second.exists = true;
		it->second.model = drawData.model;
		memcpy(it->second.transforms.data(), drawData.transforms.data(), sizeof(drawData.transforms));
	}
}

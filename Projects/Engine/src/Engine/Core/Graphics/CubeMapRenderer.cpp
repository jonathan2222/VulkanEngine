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

	this->defaultCubeModel.init(1.f);
}

void ym::CubeMapRenderer::destroy()
{
	this->defaultCubeModel.setTexture(nullptr); // The cubemodel should not destroy the texture.
	this->defaultCubeModel.destroy();

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

void ym::CubeMapRenderer::drawSkybox(uint32_t imageIndex, Texture* texture)
{
	// This can be done because there will always be only one skybox at the same time.
	this->defaultCubeModel.setTexture(texture, 1); // Also create a sampler with one mip level.
	glm::mat4 transform(1.f);

	DrawData drawData = {};
	drawData.model = &this->defaultCubeModel;
	drawData.transforms.push_back(transform);
	addModelToBatch(imageIndex, drawData);
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
			Texture* texture = drawData.second.model->getTexture();
			VkDescriptorImageInfo imageInfo = texture->descriptor;

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
	uint64_t id = getUniqueId((uint32_t)drawData.model->getUniqueId(), instanceCount);
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

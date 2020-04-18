#include "stdafx.h"
#include "ModelRenderer.h"
#include "Engine/Core/Threading/ThreadManager.h"
#include "Engine/Core/Scene/Vertex.h"
#include "Engine/Core/Vulkan/Pipeline/DescriptorSet.h"

ym::ModelRenderer::ModelRenderer()
{
	this->threadID = 0;
	this->swapChain = nullptr;
}

ym::ModelRenderer::~ModelRenderer()
{
}

void ym::ModelRenderer::init(SwapChain* swapChain, uint32_t threadID, RenderPass* renderPass, SceneDescriptors* sceneDescriptors)
{
	this->swapChain = swapChain;
	this->threadID = threadID;
	this->commandPool.init(CommandPool::Queue::GRAPHICS, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	this->commandBuffers = this->commandPool.createCommandBuffers(this->swapChain->getNumImages(), VK_COMMAND_BUFFER_LEVEL_SECONDARY);

	this->shader.addStage(Shader::Type::VERTEX, YM_ASSETS_FILE_PATH + "Shaders/pbrTestVert.spv");
	this->shader.addStage(Shader::Type::FRAGMENT, YM_ASSETS_FILE_PATH + "Shaders/pbrTestFrag.spv");
	this->shader.init();

	this->shouldRecreateDescriptors.resize(this->swapChain->getNumImages(), false);
	this->descriptorPools.resize(this->swapChain->getNumImages());
	this->sceneDescriptors = sceneDescriptors;

	createDescriptorLayouts();
	std::vector<DescriptorLayout> descriptorLayouts = { this->sceneDescriptors->layout, descriptorSetLayouts.node, descriptorSetLayouts.material, descriptorSetLayouts.model };
	VkVertexInputBindingDescription vertexBindingDescriptions = Vertex::getBindingDescriptions();
	std::array<VkVertexInputAttributeDescription, 3> vertexAttributeDescriptions = Vertex::getAttributeDescriptions();
	PipelineInfo pipelineInfo = {};
	pipelineInfo.vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	pipelineInfo.vertexInputInfo.vertexBindingDescriptionCount = 1;
	pipelineInfo.vertexInputInfo.vertexAttributeDescriptionCount = 3;
	pipelineInfo.vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDescriptions;
	pipelineInfo.vertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();

	VkPushConstantRange pushConstRange = {};
	pushConstRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstRange.size = sizeof(Material::PushData);
	pushConstRange.offset = 0;
	this->pipeline.setPushConstant(pushConstRange);
	this->pipeline.setPipelineInfo(PipelineInfoFlag::VERTEX_INPUT, pipelineInfo);
	this->pipeline.setDescriptorLayouts(descriptorLayouts);
	this->pipeline.setGraphicsPipelineInfo(this->swapChain->getExtent(), renderPass);
	this->pipeline.setWireframe(false);
	this->pipeline.init(Pipeline::Type::GRAPHICS, &this->shader);
}

void ym::ModelRenderer::destroy()
{
	this->shader.destroy();
	this->pipeline.destroy();
	this->commandPool.destroy();
	for (DescriptorPool& pool : this->descriptorPools)
		pool.destroy();

	this->descriptorSetLayouts.model.destroy();
	this->descriptorSetLayouts.node.destroy();
	this->descriptorSetLayouts.material.destroy();

	for (auto& batch : this->drawBatch)
		for (auto& drawData : batch)
			drawData.second.transformsBuffer.destroy();
}

void ym::ModelRenderer::begin(uint32_t imageIndex, VkCommandBufferInheritanceInfo inheritanceInfo)
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

void ym::ModelRenderer::drawModel(uint32_t imageIndex, Model* model, const glm::mat4& transform)
{
	DrawData drawData = {};
	drawData.model = model;
	drawData.transforms.push_back(transform);
	addModelToBatch(imageIndex, drawData);
}

void ym::ModelRenderer::drawModel(uint32_t imageIndex, Model* model, const std::vector<glm::mat4>& transforms)
{
	DrawData drawData = {};
	drawData.model = model;
	drawData.transforms.insert(drawData.transforms.begin(), transforms.begin(), transforms.end());
	addModelToBatch(imageIndex, drawData);
}

void ym::ModelRenderer::end(uint32_t imageIndex)
{
	if (this->shouldRecreateDescriptors[imageIndex])
	{
		// Fetch number of nodes and materials.
		uint32_t modelCount = 0;
		uint32_t nodeCount = 0;
		uint32_t materialCount = 0;
		for (auto& drawData : this->drawBatch[imageIndex])
		{
			modelCount++;
			nodeCount += drawData.second.model->numMeshes;
			materialCount += static_cast<uint32_t>(drawData.second.model->materials.size());
		}
		recreateDescriptorPool(imageIndex, materialCount, nodeCount, modelCount);

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
					recordModel(drawData.second.model, imageIndex, drawData.second.descriptorSet, instanceCount, currentBuffer);
					});
			}
		}

		ThreadManager::wait(this->threadID);
	};
	currentBuffer->end();
}

std::vector<ym::CommandBuffer*>& ym::ModelRenderer::getBuffers()
{
	return this->commandBuffers;
}

void ym::ModelRenderer::recordModel(Model* model, uint32_t imageIndex, VkDescriptorSet instanceDescriptorSet, uint32_t instanceCount, CommandBuffer* cmdBuffer)
{
	if (model->vertexBuffer.getBuffer() == VK_NULL_HANDLE)
		return;

	cmdBuffer->cmdBindPipeline(&this->pipeline);

	VkBuffer buffer = model->vertexBuffer.getBuffer();
	VkDeviceSize size = 0;
	cmdBuffer->cmdBindVertexBuffers(0, 1, &buffer, &size);
	if (model->indices.empty() == false)
		cmdBuffer->cmdBindIndexBuffer(model->indexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT32);

	for (Model::Node& node : model->nodes)
		drawNode(imageIndex, instanceDescriptorSet, cmdBuffer, &this->pipeline, node, instanceCount);
}

void ym::ModelRenderer::drawNode(uint32_t imageIndex, VkDescriptorSet instanceDescriptorSet, CommandBuffer* commandBuffer, Pipeline* pipeline, Model::Node& node, uint32_t instanceCount)
{
	if (node.hasMesh)
	{
		Mesh& mesh = node.mesh;
		for (Primitive& primitive : mesh.primitives)
		{
			Material::PushData& pushData = primitive.material->pushData;
			commandBuffer->cmdPushConstants(pipeline, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Material::PushData), &pushData);

			std::vector<VkDescriptorSet> sets = { 
				this->sceneDescriptors->sets[imageIndex],
				node.descriptorSets[imageIndex], 
				primitive.material->descriptorSets[imageIndex],
				instanceDescriptorSet
			};
			std::vector<uint32_t> offsets;
			commandBuffer->cmdBindDescriptorSets(pipeline, 0, sets, offsets);

			if (primitive.hasIndices)
				commandBuffer->cmdDrawIndexed(primitive.indexCount, instanceCount, primitive.firstIndex, 0, 0);
			else
				commandBuffer->cmdDraw(primitive.vertexCount, instanceCount, 0, 0);
		}
	}

	// Draw child nodes
	for (Model::Node& child : node.children)
		drawNode(imageIndex, instanceDescriptorSet, commandBuffer, pipeline, child, instanceCount);
}

void ym::ModelRenderer::createDescriptorLayouts()
{
	// Model
	descriptorSetLayouts.model.add(new SSBO(VK_SHADER_STAGE_VERTEX_BIT)); // Instance transformations.
	descriptorSetLayouts.model.init();

	// Node
	descriptorSetLayouts.node.add(new UBO(VK_SHADER_STAGE_VERTEX_BIT)); // Node transform
	descriptorSetLayouts.node.init();

	// Material
	descriptorSetLayouts.material.add(new IMG(VK_SHADER_STAGE_FRAGMENT_BIT)); // BaseColor
	descriptorSetLayouts.material.add(new IMG(VK_SHADER_STAGE_FRAGMENT_BIT)); // MetallicRoughness
	descriptorSetLayouts.material.add(new IMG(VK_SHADER_STAGE_FRAGMENT_BIT)); // NormalTexture
	descriptorSetLayouts.material.add(new IMG(VK_SHADER_STAGE_FRAGMENT_BIT)); // OcclusionTexture
	descriptorSetLayouts.material.add(new IMG(VK_SHADER_STAGE_FRAGMENT_BIT)); // EmissiveTexture
	descriptorSetLayouts.material.init();
}

void ym::ModelRenderer::recreateDescriptorPool(uint32_t imageIndex, uint32_t materialCount, uint32_t nodeCount, uint32_t modelCount)
{
	// Destroy the descriptor pool and its descriptor sets if it should be recreated.
	DescriptorPool& descriptorPool = this->descriptorPools[imageIndex];
	if (descriptorPool.wasCreated())
		descriptorPool.destroy();

	// There are many models with its own data => own descriptor set, multiply with the number of models.
	descriptorPool.addDescriptorLayout(descriptorSetLayouts.model, modelCount);
	// There are many nodes with its own data => own descriptor set, multiply with the number of nodes.
	descriptorPool.addDescriptorLayout(descriptorSetLayouts.node, nodeCount);
	// Same for the material but multiply instead with the number of materials.
	descriptorPool.addDescriptorLayout(descriptorSetLayouts.material, materialCount);

	descriptorPool.init(1);
}

void ym::ModelRenderer::createDescriptorsSets(uint32_t imageIndex, std::map<uint64_t, DrawData>& drawBatch)
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

		// Nodes
		std::vector<Model::Node>& nodes = drawData.second.model->nodes;
		for(Model::Node& node : nodes)
			createNodeDescriptorsSets(imageIndex, node);

		// Materials
		std::vector<Material>& materials = drawData.second.model->materials;
		for (Material& material : materials)
		{
			if(material.descriptorSets.empty())
				material.descriptorSets.resize(this->swapChain->getNumImages(), VK_NULL_HANDLE);

			{
				// Create descriptor sets for each material.
				std::vector<Material::Tex> textures = {
						material.baseColorTexture,
						material.metallicRoughnessTexture,
						material.normalTexture,
						material.occlusionTexture,
						material.emissiveTexture
				};

				std::array<VkDescriptorImageInfo, 5> imageInfos;
				for (size_t j = 0; j < textures.size(); j++) 
				{
					Material::Tex& tex = textures[j];
					imageInfos[j].imageLayout = tex.texture->image.getLayout();
					imageInfos[j].imageView = tex.texture->imageView.getImageView();
					imageInfos[j].sampler = tex.sampler->getSampler();
				}

				{
					DescriptorSet materialSet;
					materialSet.init(descriptorSetLayouts.material, &material.descriptorSets[imageIndex], &this->descriptorPools[imageIndex]);
					for(uint32_t binding = 0; binding < 5; binding++)
						materialSet.setImageDesc(binding, imageInfos[binding]);
					materialSet.update();
				}
			}
		}
	}
}

void ym::ModelRenderer::createNodeDescriptorsSets(uint32_t imageIndex, Model::Node& node)
{
	// Create uniform buffers if not created.
	if (node.uniformBuffers.empty())
	{
		node.uniformBuffers.resize(this->swapChain->getNumImages());
	}
	if (node.uniformBuffers[imageIndex].isInitialized() == false)
	{
		node.uniformBuffers[imageIndex].init(sizeof(Model::Node::NodeData));
		Model::Node::NodeData data;
		data.transform = node.matrix;
		node.uniformBuffers[imageIndex].transfer(&data, sizeof(data), 0);
	}

	if (node.descriptorSets.empty())
		node.descriptorSets.resize(this->swapChain->getNumImages(), VK_NULL_HANDLE);

	DescriptorSet nodeSet;
	nodeSet.init(descriptorSetLayouts.node, &node.descriptorSets[imageIndex], &this->descriptorPools[imageIndex]);
	nodeSet.setBufferDesc(0, node.uniformBuffers[imageIndex].getDescriptor());
	nodeSet.update();

	for (Model::Node& child : node.children)
		createNodeDescriptorsSets(imageIndex, child);
}

uint64_t ym::ModelRenderer::getUniqueId(uint32_t modelId, uint32_t instanceCount)
{
	uint64_t instanceCount64 = static_cast<uint64_t>(instanceCount) & 0xFFFFFFFF;
	uint64_t modelId64 = static_cast<uint64_t>(modelId) & 0xFFFFFFFF;
	uint64_t id = (modelId64 << 32) | instanceCount64;
	return id;
}

void ym::ModelRenderer::addModelToBatch(uint32_t imageIndex, DrawData& drawData)
{
	// Only draw model if it is ready.
	if (drawData.model->hasLoaded)
	{
		if (this->drawBatch.empty())
			this->drawBatch.resize(this->swapChain->getNumImages());

		uint32_t instanceCount = (uint32_t)drawData.transforms.size();
		uint64_t id = getUniqueId(drawData.model->uniqueId, instanceCount);
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
}

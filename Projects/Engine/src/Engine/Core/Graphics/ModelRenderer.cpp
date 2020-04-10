#include "stdafx.h"
#include "ModelRenderer.h"
#include "Engine/Core/Threading/ThreadManager.h"

ym::ModelRenderer::ModelRenderer()
{
	this->threadID = 0;
	this->swapChain = nullptr;
}

ym::ModelRenderer::~ModelRenderer()
{
}

ym::ModelRenderer* ym::ModelRenderer::get()
{
	static ModelRenderer modelRenderer;
	return &modelRenderer;
}

void ym::ModelRenderer::init(SwapChain* swapChain, uint32_t threadID, RenderPass* renderPass)
{
	this->swapChain = swapChain;
	this->threadID = threadID;
	this->commandPool.init(CommandPool::Queue::GRAPHICS, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	this->commandBuffers = this->commandPool.createCommandBuffers(this->swapChain->getNumImages(), VK_COMMAND_BUFFER_LEVEL_SECONDARY);

	this->shader.addStage(Shader::Type::VERTEX, YM_ASSETS_FILE_PATH + "Shaders/pbrTestVert.spv");
	this->shader.addStage(Shader::Type::FRAGMENT, YM_ASSETS_FILE_PATH + "Shaders/pbrTestFrag.spv");
	this->shader.init();

	this->shouldRecreateDescriptors.resize(this->swapChain->getNumImages(), false);
	this->descriptorPools.resize(this->swapChain->getNumImages(), VK_NULL_HANDLE);
	this->descriptorSets.resize(this->swapChain->getNumImages());

	createDescriptorLayouts();
	std::vector<DescriptorLayout> descriptorLayouts = { descriptorSetLayouts.scene, descriptorSetLayouts.node, descriptorSetLayouts.material, descriptorSetLayouts.model };
	VkVertexInputBindingDescription vertexBindingDescriptions = Vertex::getBindingDescriptions();
	std::array<VkVertexInputAttributeDescription, 3> vertexAttributeDescriptions = Vertex::getAttributeDescriptions();
	PipelineInfo pipelineInfo = {};
	pipelineInfo.vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	pipelineInfo.vertexInputInfo.vertexBindingDescriptionCount = 1;
	pipelineInfo.vertexInputInfo.vertexAttributeDescriptionCount = 3;
	pipelineInfo.vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDescriptions;
	pipelineInfo.vertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();

	this->pipeline.setPipelineInfo(PipelineInfoFlag::VERTEX_INPUT, pipelineInfo);
	this->pipeline.setDescriptorLayouts(descriptorLayouts);
	this->pipeline.setGraphicsPipelineInfo(this->swapChain->getExtent(), renderPass);
	this->pipeline.setWireframe(false);
	this->pipeline.init(Pipeline::Type::GRAPHICS, &this->shader);

	createBuffers();
}

void ym::ModelRenderer::destroy()
{
	this->shader.destroy();
	this->pipeline.destroy();
	this->commandPool.destroy();
	for(VkDescriptorPool pool : this->descriptorPools)
		vkDestroyDescriptorPool(VulkanInstance::get()->getLogicalDevice(), pool, nullptr);

	;
	vkDestroyDescriptorSetLayout(VulkanInstance::get()->getLogicalDevice(), this->descriptorSetLayouts.scene.getLayout(), nullptr);
	vkDestroyDescriptorSetLayout(VulkanInstance::get()->getLogicalDevice(), this->descriptorSetLayouts.model.getLayout(), nullptr);
	vkDestroyDescriptorSetLayout(VulkanInstance::get()->getLogicalDevice(), this->descriptorSetLayouts.node.getLayout(), nullptr);
	vkDestroyDescriptorSetLayout(VulkanInstance::get()->getLogicalDevice(), this->descriptorSetLayouts.material.getLayout(), nullptr);

	for (auto& batch : this->drawBatch)
		for (auto& drawData : batch)
			drawData.second.transformsBuffer.destroy();

	for (UniformBuffer& ubo : this->sceneUBOs)
		ubo.destroy();
}

void ym::ModelRenderer::setCamera(Camera* camera)
{
	this->activeCamera = camera;
}

void ym::ModelRenderer::begin(uint32_t imageIndex, VkCommandBufferInheritanceInfo inheritanceInfo)
{
	this->inheritanceInfo = inheritanceInfo;

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
	// Update camera.
	SceneData sceneData;
	sceneData.proj = this->activeCamera->getProjection();
	sceneData.view = this->activeCamera->getView();
	sceneData.cPos = this->activeCamera->getPosition();
	this->sceneUBOs[imageIndex].transfer(&sceneData, sizeof(SceneData), 0);

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
	CommandBuffer* currentBuffer = commandBuffers[imageIndex];
	currentBuffer->begin(VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, &inheritanceInfo);
	for (auto& drawData : this->drawBatch[imageIndex])
	{
		uint32_t instanceCount = (uint32_t)drawData.second.transforms.size();
		drawData.second.transformsBuffer.transfer(drawData.second.transforms.data(), sizeof(glm::mat4)*instanceCount, 0);
		ThreadManager::addWork(this->threadID, [=]() { 
			recordModel(drawData.second.model, imageIndex, drawData.second.descriptorSet, instanceCount, currentBuffer, inheritanceInfo);
		});
	}

	ThreadManager::wait(this->threadID);
	currentBuffer->end();
}

std::vector<ym::CommandBuffer*>& ym::ModelRenderer::getBuffers()
{
	return this->commandBuffers;
}

void ym::ModelRenderer::recordModel(Model* model, uint32_t imageIndex, VkDescriptorSet instanceDescriptorSet, uint32_t instanceCount, CommandBuffer* cmdBuffer, VkCommandBufferInheritanceInfo inheritanceInfo)
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
		// Set transformation matrix
		//PushConstantData pushConstantData;
		//pushConstantData.matrix = node.parent != nullptr ? (transform * node.parent->matrix * node.matrix) : (transform * node.matrix);
		//commandBuffer->cmdPushConstants(pipeline, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantData), &pushConstantData);

		Mesh& mesh = node.mesh;
		for (Primitive& primitive : mesh.primitives)
		{
			//Material::PushData& pushData = primitive.material->pushData;
			//commandBuffer->cmdPushConstants(pipeline, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(PushConstantData), sizeof(Material::PushData), &pushData);

			std::vector<VkDescriptorSet> sets = { 
				this->descriptorSets[imageIndex].scene, 
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

void ym::ModelRenderer::createBuffers()
{
	// Scene buffer
	this->sceneUBOs.resize(this->swapChain->getNumImages());
	for(auto& ubo : this->sceneUBOs)
		ubo.init(sizeof(SceneData));
}

void ym::ModelRenderer::createDescriptorLayouts()
{
	// Scene
	descriptorSetLayouts.scene.add(new UBO(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)); // Camera
	descriptorSetLayouts.scene.init();

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
	if (this->descriptorPools[imageIndex] != VK_NULL_HANDLE)
		vkDestroyDescriptorPool(VulkanInstance::get()->getLogicalDevice(), this->descriptorPools[imageIndex], nullptr);

	uint32_t numFrames = this->swapChain->getNumImages();

	// Function to fill poolSizesMap with new data. (Accumulative)
	std::unordered_map<VkDescriptorType, uint32_t> poolSizesMap;
	auto addToMap = [&](std::vector<VkDescriptorPoolSize> & poolSizes, uint32_t multiplier) {
		for (VkDescriptorPoolSize& s : poolSizes)
		{
			auto& it = poolSizesMap.find(s.type);
			if (it == poolSizesMap.end()) poolSizesMap[s.type] = s.descriptorCount * multiplier;
			else it->second += s.descriptorCount * multiplier;
		}
	};

	// Add pool sizes from each descriptor set.
	addToMap(descriptorSetLayouts.scene.getPoolSizes(1), 1);
	// There are many models with its own data => own descriptor set, multiply with the number of models.
	addToMap(descriptorSetLayouts.model.getPoolSizes(1), modelCount);
	// There are many nodes with its own data => own descriptor set, multiply with the number of nodes.
	addToMap(descriptorSetLayouts.node.getPoolSizes(1), nodeCount);
	// Same for the material but multiply instead with the number of materials.
	addToMap(descriptorSetLayouts.material.getPoolSizes(1), materialCount);

	std::vector<VkDescriptorPoolSize> poolSizes;
	for (auto m : poolSizesMap)
		poolSizes.push_back({ m.first, m.second * numFrames });

	// Create descriptor pool.
	VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	descriptorPoolInfo.pPoolSizes = poolSizes.data();
	descriptorPoolInfo.maxSets = (1 + modelCount + nodeCount + materialCount) * numFrames;
	VULKAN_CHECK(vkCreateDescriptorPool(VulkanInstance::get()->getLogicalDevice(), &descriptorPoolInfo, nullptr, &this->descriptorPools[imageIndex]), "Could not create descriptor pool!");
}

void ym::ModelRenderer::createDescriptorsSets(uint32_t imageIndex, std::map<uint64_t, DrawData>& drawBatch)
{
	VkDevice device = VulkanInstance::get()->getLogicalDevice();

	// Scene
	{
		VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
		descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocInfo.descriptorPool = this->descriptorPools[imageIndex];
		VkDescriptorSetLayout descriptorSetLayout = descriptorSetLayouts.scene.getLayout();
		descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;
		descriptorSetAllocInfo.descriptorSetCount = 1;
		VULKAN_CHECK(vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, &this->descriptorSets[imageIndex].scene), "Could not create descriptor set for the scene!");

		std::array<VkWriteDescriptorSet, 1> writeDescriptorSets{};

		writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescriptorSets[0].descriptorCount = 1;
		writeDescriptorSets[0].dstSet = descriptorSets[imageIndex].scene;
		writeDescriptorSets[0].dstBinding = 0;
		VkDescriptorBufferInfo descriptor = sceneUBOs[imageIndex].getDescriptor();
		writeDescriptorSets[0].pBufferInfo = &descriptor;

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
	}

	// Nodes, Materials and Models
	for (auto& drawData : drawBatch)
	{
		// Models
		{
			VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
			descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descriptorSetAllocInfo.descriptorPool = this->descriptorPools[imageIndex];
			VkDescriptorSetLayout descriptorSetLayout = descriptorSetLayouts.model.getLayout();
			descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;
			descriptorSetAllocInfo.descriptorSetCount = 1;
			VULKAN_CHECK(vkAllocateDescriptorSets(VulkanInstance::get()->getLogicalDevice(), &descriptorSetAllocInfo, &drawData.second.descriptorSet), "Could not create descriptor set for the model!");

			std::array<VkWriteDescriptorSet, 1> writeDescriptorSets{};

			writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writeDescriptorSets[0].descriptorCount = 1;
			writeDescriptorSets[0].dstSet = drawData.second.descriptorSet;
			writeDescriptorSets[0].dstBinding = 0;
			VkDescriptorBufferInfo descriptor = drawData.second.transformsBuffer.getDescriptor();
			writeDescriptorSets[0].pBufferInfo = &descriptor;

			vkUpdateDescriptorSets(VulkanInstance::get()->getLogicalDevice(), static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
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

			//if (material.descriptorSets[imageIndex] == VK_NULL_HANDLE)
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

				//for (uint32_t i = 0; i < this->swapChain->getNumImages(); i++)
				{
					VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
					descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
					descriptorSetAllocInfo.descriptorPool = descriptorPools[imageIndex];
					VkDescriptorSetLayout descriptorSetLayout = descriptorSetLayouts.material.getLayout();
					descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;
					descriptorSetAllocInfo.descriptorSetCount = 1;
					VULKAN_CHECK(vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, &material.descriptorSets[imageIndex]), "Could not create descriptor set for material!");

					/*std::array<VkWriteDescriptorSet, 5> writeDescriptorSets{};
					for (size_t j = 0; j < textures.size(); j++) {

						Material::Tex& tex = textures[j];
						VkDescriptorImageInfo imageInfo;
						imageInfo.imageLayout = tex.texture->image.getLayout();
						imageInfo.imageView = tex.texture->imageView.getImageView();
						imageInfo.sampler = tex.sampler->getSampler();

						writeDescriptorSets[j] = {};
						writeDescriptorSets[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						writeDescriptorSets[j].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
						writeDescriptorSets[j].descriptorCount = 1;
						writeDescriptorSets[j].dstSet = material.descriptorSets[i];
						writeDescriptorSets[j].dstBinding = static_cast<uint32_t>(j);
						writeDescriptorSets[j].pImageInfo = &imageInfo;
					}
					vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
					*/

					VkWriteDescriptorSet writeDescriptorSet = {};
					writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					writeDescriptorSet.descriptorCount = static_cast<uint32_t>(imageInfos.size());
					writeDescriptorSet.dstSet = material.descriptorSets[imageIndex];
					writeDescriptorSet.dstBinding = 0;
					writeDescriptorSet.pImageInfo = imageInfos.data();

					vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, NULL);
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
	VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
	descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocInfo.descriptorPool = this->descriptorPools[imageIndex];
	VkDescriptorSetLayout descriptorSetLayout = descriptorSetLayouts.node.getLayout();
	descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;
	descriptorSetAllocInfo.descriptorSetCount = 1;
	VULKAN_CHECK(vkAllocateDescriptorSets(VulkanInstance::get()->getLogicalDevice(), &descriptorSetAllocInfo, &node.descriptorSets[imageIndex]), "Could not create descriptor set for the node!");
	
	std::array<VkWriteDescriptorSet, 1> writeDescriptorSets{};

	writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeDescriptorSets[0].descriptorCount = 1;
	writeDescriptorSets[0].dstSet = node.descriptorSets[imageIndex];
	writeDescriptorSets[0].dstBinding = 0;
	VkDescriptorBufferInfo descriptor = node.uniformBuffers[imageIndex].getDescriptor();
	writeDescriptorSets[0].pBufferInfo = &descriptor;

	vkUpdateDescriptorSets(VulkanInstance::get()->getLogicalDevice(), static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
	
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

		// Tell renderer to recreate descriptors before rendering.
		this->shouldRecreateDescriptors[imageIndex] = true;
	}
	else
	{
		it->second.model = drawData.model;
		memcpy(it->second.transforms.data(), drawData.transforms.data(), sizeof(drawData.transforms));
	}
}

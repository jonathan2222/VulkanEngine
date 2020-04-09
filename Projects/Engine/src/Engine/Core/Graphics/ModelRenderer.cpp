#include "stdafx.h"
#include "ModelRenderer.h"
#include "Engine/Core/Threading/ThreadManager.h"

ym::ModelRenderer::ModelRenderer()
{
	this->threadID = 0;
	this->swapChain = nullptr;
	this->descriptorPool = VK_NULL_HANDLE;
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
	this->commandPool.init(CommandPool::Queue::GRAPHICS, 0);
	this->commandBuffers = this->commandPool.createCommandBuffers(this->swapChain->getNumImages(), VK_COMMAND_BUFFER_LEVEL_SECONDARY);

	this->shader.addStage(Shader::Type::VERTEX, YM_ASSETS_FILE_PATH + "Shaders/pbrVert.spv");
	this->shader.addStage(Shader::Type::FRAGMENT, YM_ASSETS_FILE_PATH + "Shaders/pbrFrag.spv");
	this->shader.init();

	createDescriptorLayouts();
	std::vector<DescriptorLayout> descriptorLayouts = { descriptorSetLayouts.scene, descriptorSetLayouts.node, descriptorSetLayouts.material };
	this->pipeline.setDescriptorLayouts(descriptorLayouts);
	this->pipeline.setGraphicsPipelineInfo(this->swapChain->getExtent(), renderPass);
	this->pipeline.setWireframe(false);
	this->pipeline.init(Pipeline::Type::GRAPHICS, &this->shader);
}

void ym::ModelRenderer::destroy()
{
	this->commandPool.destroy();
	vkDestroyDescriptorPool(VulkanInstance::get()->getLogicalDevice(), this->descriptorPool, nullptr);
}

void ym::ModelRenderer::begin(VkCommandBufferInheritanceInfo inheritanceInfo)
{
	this->inheritanceInfo = inheritanceInfo;
	this->drawBatch.clear();
	this->uniqueId = 0;
}

void ym::ModelRenderer::drawModel(Model* model, const glm::mat4& transform)
{
	DrawData drawData = {};
	drawData.model = model;
	drawData.transforms.push_back(transform);
	this->drawBatch[this->uniqueId] = drawData;
	this->uniqueId++;
}

void ym::ModelRenderer::drawModel(Model* model, const std::vector<glm::mat4>& transforms)
{
	DrawData drawData = {};
	drawData.model = model;
	drawData.transforms.insert(drawData.transforms.begin(), transforms.begin(), transforms.end());
	this->drawBatch[this->uniqueId] = drawData;
	this->uniqueId++;
}

void ym::ModelRenderer::end(uint32_t imageIndex)
{
	// Check if new data should be drawn. If so, update descriptors.
	// - Make Model hold a pointer to a mesh. (Can use same mesh, but different transforms)
	// - Use the Model ptr together with uniqueId to check if same.
	// - 

	/* TODO: 
		- Create model transform buffer.
		- Descriptor for the model's vertex buffer.
		- Solve problem of when to recreate descriptor pool.
	*/

	// If not the same, recreate the descriptor pool.
	bool shouldRecreatePool = false;
	if (shouldRecreatePool)
	{
		// Recreate the descriptor pool on the other thread.
		ThreadManager::addWork(this->threadID, [=]() {
			// Fetch number of nodes and materials.
			uint32_t nodeCount = 0;
			uint32_t materialCount = 0;
			for (auto& drawData : this->drawBatch)
			{
				nodeCount += drawData.second.model->numMeshes;
				materialCount += static_cast<uint32_t>(drawData.second.model->materials.size());
			}
			recreateDescriptorPool(materialCount, nodeCount);

			// Recreate descriptor sets
			createDescriptorsSets(this->drawBatch);
		});
	}

	// Start recording all draw commands on the thread.
	CommandBuffer* currentBuffer = commandBuffers[imageIndex];
	currentBuffer->begin(VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, &inheritanceInfo);
	for (auto& drawData : this->drawBatch)
	{
		ThreadManager::addWork(this->threadID, [=]() { 
			recordModel(drawData.second.model, drawData.second.transforms, imageIndex, currentBuffer, inheritanceInfo);
		});
	}
	currentBuffer->end();
}

std::vector<ym::CommandBuffer*>& ym::ModelRenderer::getBuffers()
{
	return this->commandBuffers;
}

void ym::ModelRenderer::recordModel(Model* model, std::vector<glm::mat4> transform, uint32_t imageIndex, CommandBuffer* cmdBuffer, VkCommandBufferInheritanceInfo inheritanceInfo)
{
	if (model->vertexBuffer.getBuffer() == VK_NULL_HANDLE)
		return;
	
	cmdBuffer->cmdBindPipeline(&this->pipeline);

	if (model->indices.empty() == false)
		cmdBuffer->cmdBindIndexBuffer(model->indexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT32);

	// Update transforms

	for (Model::Node& node : model->nodes)
		drawNode(imageIndex, cmdBuffer, &this->pipeline, node, 1);
}

void ym::ModelRenderer::drawNode(uint32_t imageIndex, CommandBuffer* commandBuffer, Pipeline* pipeline, Model::Node& node, uint32_t instanceCount)
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

			std::vector<VkDescriptorSet> sets = { this->descriptorSets[imageIndex].scene, node.descriptorSets[imageIndex], primitive.material->descriptorSets[imageIndex] };
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
		drawNode(imageIndex, commandBuffer, pipeline, child, instanceCount);
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

void ym::ModelRenderer::recreateDescriptorPool(uint32_t materialCount, uint32_t nodeCount)
{
	// Destroy the descriptor pool and its descriptor sets if it should be recreated.
	if (this->descriptorPool != VK_NULL_HANDLE)
		vkDestroyDescriptorPool(VulkanInstance::get()->getLogicalDevice(), this->descriptorPool, nullptr);

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
	descriptorPoolInfo.maxSets = (1 + nodeCount + materialCount) * numFrames;
	VULKAN_CHECK(vkCreateDescriptorPool(VulkanInstance::get()->getLogicalDevice(), &descriptorPoolInfo, nullptr, &this->descriptorPool), "Could not create descriptor pool!");
}

void ym::ModelRenderer::createDescriptorsSets(std::map<uint64_t, DrawData>& drawBatch)
{
	VkDevice device = VulkanInstance::get()->getLogicalDevice();

	// Scene
	this->descriptorSets.resize(this->swapChain->getNumImages());
	for(size_t i = 0; i < this->descriptorSets.size(); i++)
	{
		VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
		descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocInfo.descriptorPool = this->descriptorPool;
		VkDescriptorSetLayout descriptorSetLayout = descriptorSetLayouts.scene.getLayout();
		descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;
		descriptorSetAllocInfo.descriptorSetCount = 1;
		VULKAN_CHECK(vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, &this->descriptorSets[i].scene), "Could not create descriptor set [{0}] for the scene!", i);

		std::array<VkWriteDescriptorSet, 1> writeDescriptorSets{};

		writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescriptorSets[0].descriptorCount = 1;
		writeDescriptorSets[0].dstSet = descriptorSets[i].scene;
		writeDescriptorSets[0].dstBinding = 0;
		VkDescriptorBufferInfo descriptor = sceneUBOs[i].getDescriptor();
		writeDescriptorSets[0].pBufferInfo = &descriptor;

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
	}

	// Nodes and Materials
	for (auto& drawData : drawBatch)
	{
		// Nodes
		std::vector<Model::Node>& nodes = drawData.second.model->nodes;
		for(Model::Node& node : nodes)
			createNodeDescriptorsSets(node);

		// Materials
		std::vector<Material>& materials = drawData.second.model->materials;
		for (Material& material : materials)
		{
			if (material.descriptorSets.empty())
			{
				// Create descriptor sets for each material.
				material.descriptorSets.resize(this->swapChain->getNumImages());
				for (uint32_t i = 0; i < this->swapChain->getNumImages(); i++)
				{
					VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
					descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
					descriptorSetAllocInfo.descriptorPool = descriptorPool;
					VkDescriptorSetLayout descriptorSetLayout = descriptorSetLayouts.material.getLayout();
					descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;
					descriptorSetAllocInfo.descriptorSetCount = 1;
					VULKAN_CHECK(vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, &material.descriptorSets[i]), "Could not create descriptor set for material!");
				
					std::vector<Material::Tex> textures = {
						material.baseColorTexture,
						material.metallicRoughnessTexture,
						material.normalTexture,
						material.occlusionTexture,
						material.emissiveTexture
					};

					std::array<VkWriteDescriptorSet, 5> writeDescriptorSets{};
					for (size_t j = 0; j < textures.size(); j++) {

						Material::Tex& tex = textures[j];
						VkDescriptorImageInfo imageInfo;
						imageInfo.imageLayout = tex.texture->image.getLayout();
						imageInfo.imageView = tex.texture->imageView.getImageView();
						imageInfo.sampler = tex.sampler->getSampler();

						writeDescriptorSets[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						writeDescriptorSets[j].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
						writeDescriptorSets[j].descriptorCount = 1;
						writeDescriptorSets[j].dstSet = material.descriptorSets[i];
						writeDescriptorSets[j].dstBinding = static_cast<uint32_t>(j);
						writeDescriptorSets[j].pImageInfo = &imageInfo;
					}

					vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
				}
			}
		}
	}
}

void ym::ModelRenderer::createNodeDescriptorsSets(Model::Node& node)
{
	// Create uniform buffers if not created.
	if (node.uniformBuffers.empty())
	{
		node.uniformBuffers.resize(this->swapChain->getNumImages());
		for (uint32_t i = 0; i < this->swapChain->getNumImages(); i++)
		{
			node.uniformBuffers[i].init(sizeof(Model::Node::NodeData));
			Model::Node::NodeData data;
			data.transform = node.matrix;
			node.uniformBuffers[i].transfer(&data, sizeof(data), 0);
		}
	}

	for (uint32_t i = 0; i < this->swapChain->getNumImages(); i++)
	{
		VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
		descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocInfo.descriptorPool = this->descriptorPool;
		VkDescriptorSetLayout descriptorSetLayout = descriptorSetLayouts.node.getLayout();
		descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;
		descriptorSetAllocInfo.descriptorSetCount = 1;
		VULKAN_CHECK(vkAllocateDescriptorSets(VulkanInstance::get()->getLogicalDevice(), &descriptorSetAllocInfo, &node.descriptorSets[i]), "Could not create descriptor set [{0}] for the node!", i);
	
		std::array<VkWriteDescriptorSet, 1> writeDescriptorSets{};

		writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescriptorSets[0].descriptorCount = 1;
		writeDescriptorSets[0].dstSet = descriptorSets[i].scene;
		writeDescriptorSets[0].dstBinding = 0;
		VkDescriptorBufferInfo descriptor = node.uniformBuffers[i].getDescriptor();
		writeDescriptorSets[0].pBufferInfo = &descriptor;

		vkUpdateDescriptorSets(VulkanInstance::get()->getLogicalDevice(), static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
	}
	for (Model::Node& child : node.children)
		createNodeDescriptorsSets(child);
}

#include "stdafx.h"
#include "TerrainRenderer.h"

#include "Engine/Core/Threading/ThreadManager.h"
#include "Engine/Core/Vulkan/Pipeline/DescriptorSet.h"
#include "Engine/Core/Vulkan/Factory.h"
#include "Engine/Core/Vulkan/DebugHelper.h"

ym::TerrainRenderer::TerrainRenderer()
{
	this->threadID = 0;
	this->swapChain = nullptr;
}

ym::TerrainRenderer::~TerrainRenderer()
{
}

void ym::TerrainRenderer::init(SwapChain* swapChain, uint32_t threadID, RenderPass* renderPass, SceneDescriptors* sceneDescriptors)
{
	this->swapChain = swapChain;
	this->threadID = threadID;
	this->sceneDescriptors = sceneDescriptors;
	this->commandPoolGraphics.init(CommandPool::Queue::GRAPHICS, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	this->commandPoolCompute.init(CommandPool::Queue::COMPUTE, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	this->commanPoolTransfer.init(CommandPool::Queue::TRANSFER, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	this->commandBuffersGraphics = this->commandPoolGraphics.createCommandBuffers(this->swapChain->getNumImages(), VK_COMMAND_BUFFER_LEVEL_SECONDARY);
	this->commandBuffersCompute = this->commandPoolCompute.createCommandBuffers(this->swapChain->getNumImages(), VK_COMMAND_BUFFER_LEVEL_SECONDARY);
	this->shouldRecreateDescriptors.resize(this->swapChain->getNumImages(), false);
	this->descriptorPools.resize(this->swapChain->getNumImages());

	NameBinder::bindObject((uint64_t)this->commandPoolGraphics.getCommandPool(), "Graphics Command Pool [Terrain]");
	NameBinder::bindObject((uint64_t)this->commandPoolCompute.getCommandPool(), "Compute Command Pool [Terrain]");
	NameBinder::bindObject((uint64_t)this->commanPoolTransfer.getCommandPool(), "Transfer Command Pool [Terrain]");

	for (uint32_t i = 0; i < this->swapChain->getNumImages(); i++)
	{
		std::string idStr = "[" + std::to_string(i) + "]";
		CommandBuffer* graphicsBuff = this->commandBuffersGraphics[i];
		NameBinder::bindObject((uint64_t)graphicsBuff->getCommandBuffer(), "Graphics Command Buffer [Terrain] " + idStr);
		CommandBuffer* computeBuff = this->commandBuffersCompute[i];
		NameBinder::bindObject((uint64_t)computeBuff->getCommandBuffer(), "Compute Command Buffer [Terrain] " + idStr);
	}

	createDescriptorLayouts();

	// Frustum pipeline
	{
		this->frustumShader.addStage(Shader::Type::COMPUTE, YM_ASSETS_FILE_PATH + "Shaders/terrainComp.spv");
		this->frustumShader.init();

		std::vector<DescriptorLayout> descriptorLayouts = { this->descriptorSetLayouts.frustum };
		this->frustumPipeline.setDescriptorLayouts(descriptorLayouts);
		this->frustumPipeline.init(Pipeline::Type::COMPUTE, &this->frustumShader);
	}

	// Graphics pipeline
	{
		this->shader.addStage(Shader::Type::VERTEX, YM_ASSETS_FILE_PATH + "Shaders/terrainVert.spv");
		this->shader.addStage(Shader::Type::FRAGMENT, YM_ASSETS_FILE_PATH + "Shaders/terrainFragTest.spv");
		this->shader.init();

		std::vector<DescriptorLayout> descriptorLayouts = { this->sceneDescriptors->layout, this->descriptorSetLayouts.terrain, this->descriptorSetLayouts.material };
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
		this->pipeline.setGraphicsPipelineInfo(swapChain->getExtent(), renderPass);
		//this->pipeline.setWireframe(true);
		this->pipeline.init(Pipeline::Type::GRAPHICS, &this->shader);
	}

	this->regionSize = Config::get()->fetch<int>("Terrain/regionSize");
	this->proximityRadius = Config::get()->fetch<int>("Terrain/proximityRadius");
	Terrain::ProximityDescription proxDesc = Terrain::createProximityDescription(0, this->regionSize, this->proximityRadius);
	this->indiciesPerRegion = proxDesc.indiciesPerRegion;
	this->proximityWidth = proxDesc.proximityWidth;
	generateIndicies(regionSize, proximityRadius);
	createBuffers();
}

void ym::TerrainRenderer::destroy()
{
	if (this->drawBatch.empty() == false)
	{
		for (uint32_t i = 0; i < this->swapChain->getNumImages(); i++)
			clearDrawBatch(i);
	}

	for (UniformBuffer& ubo : this->frustumPlanesUBOs)
		ubo.destroy();

	for (auto& pool : this->descriptorPools)
		pool.destroy();

	this->vertexBuffer1.destroy();
	this->vertexBuffer2.destroy();
	this->memoryDeviceLocal.destroy();
	this->stagingBuffer.destroy();
	this->memoryHost.destroy();
	this->indexBuffer.destroy();

	this->descriptorSetLayouts.terrain.destroy();
	this->descriptorSetLayouts.material.destroy();
	this->descriptorSetLayouts.frustum.destroy();
	this->commandPoolGraphics.destroy();
	this->commandPoolCompute.destroy();
	this->commanPoolTransfer.destroy();
	this->pipeline.destroy();
	this->shader.destroy();
	this->frustumPipeline.destroy();
	this->frustumShader.destroy();
}

void ym::TerrainRenderer::setActiveCamera(Camera* camera)
{
	this->activeCamera = camera;
}

void ym::TerrainRenderer::begin(uint32_t imageIndex, VkCommandBufferInheritanceInfo inheritanceInfo)
{
	this->inheritanceInfo = inheritanceInfo;

	if (this->drawBatch.empty() == false)
	{
		for (auto& drawData : this->drawBatch[imageIndex])
			drawData.second.exists = false;
	}

	// Clear draw data if descriptor pool should be recreated.
	if (this->shouldRecreateDescriptors[imageIndex])
		clearDrawBatch(imageIndex);
}

void ym::TerrainRenderer::drawTerrain(uint32_t imageIndex, Terrain* terrain, const glm::mat4& transform)
{
	DrawData drawData;
	drawData.exists = true;
	drawData.terrain = terrain;
	drawData.transform = transform;
	addTerrainToBatch(imageIndex, drawData);
}

void ym::TerrainRenderer::end(uint32_t imageIndex)
{
	this->frustumPlanesUBOs[imageIndex].transfer(this->activeCamera->getPlanes().data(), sizeof(PlanesData), 0);

	if (this->shouldRecreateDescriptors[imageIndex])
	{
		uint32_t terrainCount = 0;
		for (auto& drawData : this->drawBatch[imageIndex])
			terrainCount++;
		recreateDescriptorPool(imageIndex, terrainCount);

		// Recreate descriptor sets
		createDescriptorsSets(imageIndex, this->drawBatch[imageIndex]);

		// Update vertex buffer.
		verticesToDevice(this->activeVertexBuffer, this->vertices);

		this->shouldRecreateDescriptors[imageIndex] = false;
	}

	// Start recording all draw commands on the thread.
	CommandBuffer* currentBufferCompute = this->commandBuffersCompute[imageIndex];
	currentBufferCompute->begin(0, &this->inheritanceInfo);

	if (this->drawBatch.empty() == false)
	{
		for (auto& batch : this->drawBatch[imageIndex])
		{
			DrawData* drawData = &batch.second;
			if (drawData->exists)
			{
				ThreadManager::addWork(this->threadID, [=]() {
					recordFrustum(drawData, imageIndex, currentBufferCompute);
				});
			}
		}
		ThreadManager::wait(this->threadID);
	};
	currentBufferCompute->end();

	CommandBuffer* currentBufferGraphics = this->commandBuffersGraphics[imageIndex];
	currentBufferGraphics->begin(VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, &this->inheritanceInfo);
	if (this->drawBatch.empty() == false)
	{
		for (auto& batch : this->drawBatch[imageIndex])
		{
			DrawData* drawData = &batch.second;
			if (drawData->exists)
			{
				ThreadManager::addWork(this->threadID, [=]() {
					recordTerrain(drawData, imageIndex, currentBufferGraphics);
				});
			}
		}

		ThreadManager::wait(this->threadID);
	};
	currentBufferGraphics->end();
}

void ym::TerrainRenderer::preRecordGraphics(uint32_t imageIndex, CommandBuffer* commandBuffer)
{
	if (this->drawBatch.empty() == false)
	{
		for (auto& batch : this->drawBatch[imageIndex])
		{
			DrawData* drawData = &batch.second;
			if (drawData->exists)
			{
				commandBuffer->acquireBuffer(&drawData->indirectDrawBuffer, VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
					VulkanInstance::get()->getComputeQueue().queueIndex, VulkanInstance::get()->getGraphicsQueue().queueIndex,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);
			}
		}
	};
}

void ym::TerrainRenderer::postRecordGraphics(uint32_t imageIndex, CommandBuffer* commandBuffer)
{
	if (this->drawBatch.empty() == false)
	{
		for (auto& batch : this->drawBatch[imageIndex])
		{
			DrawData* drawData = &batch.second;
			if (drawData->exists)
			{
				commandBuffer->releaseBuffer(&drawData->indirectDrawBuffer, VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
					VulkanInstance::get()->getGraphicsQueue().queueIndex, VulkanInstance::get()->getComputeQueue().queueIndex,
					VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
			}
		}
	};
}

void ym::TerrainRenderer::preRecordCompute(uint32_t imageIndex, CommandBuffer* commandBuffer)
{
	if (this->drawBatch.empty() == false)
	{
		for (auto& batch : this->drawBatch[imageIndex])
		{
			DrawData* drawData = &batch.second;
			if (drawData->exists)
			{
				commandBuffer->acquireBuffer(&drawData->indirectDrawBuffer, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
					VulkanInstance::get()->getGraphicsQueue().queueIndex, VulkanInstance::get()->getComputeQueue().queueIndex,
					VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
			}
		}
	};
}

void ym::TerrainRenderer::postRecordCompute(uint32_t imageIndex, CommandBuffer* commandBuffer)
{
	if (this->drawBatch.empty() == false)
	{
		for (auto& batch : this->drawBatch[imageIndex])
		{
			DrawData* drawData = &batch.second;
			if (drawData->exists)
			{
				commandBuffer->releaseBuffer(&drawData->indirectDrawBuffer, VK_ACCESS_SHADER_WRITE_BIT,
					VulkanInstance::get()->getComputeQueue().queueIndex, VulkanInstance::get()->getGraphicsQueue().queueIndex,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);
			}
		}
	};
}

std::vector<ym::CommandBuffer*>& ym::TerrainRenderer::getGraphicsBuffers()
{
	return this->commandBuffersGraphics;
}

std::vector<ym::CommandBuffer*>& ym::TerrainRenderer::getComputeBuffers()
{
	return this->commandBuffersCompute;
}

void ym::TerrainRenderer::clearDrawBatch(uint32_t imageIndex)
{
	for (auto& drawData : this->drawBatch[imageIndex])
	{
		drawData.second.terrainUbo.destroy();
		drawData.second.indirectDrawBuffer.destroy();
		drawData.second.memoryDeviceLocal.destroy();
		drawData.second.proximityDataUBO.destroy();
	}
	this->drawBatch[imageIndex].clear();
}

void ym::TerrainRenderer::recordTerrain(DrawData* drawData, uint32_t imageIndex, CommandBuffer* cmdBuffer)
{
	VkDescriptorSet& descriptorSet = drawData->terrain->getDescriptorSet();
	cmdBuffer->cmdBindPipeline(&this->pipeline);
	// TODO: Add material!
	std::vector<VkDescriptorSet> sets = { this->sceneDescriptors->sets[imageIndex], descriptorSet };
	std::vector<uint32_t> offsets;
	cmdBuffer->cmdBindDescriptorSets(&this->pipeline, 0, sets, offsets);
	VkBuffer vertexBuffer = this->activeVertexBuffer->getBuffer();
	VkDeviceSize offset = 0;
	cmdBuffer->cmdBindIndexBuffer(this->indexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT32);
	cmdBuffer->cmdBindVertexBuffers(0, 1, &vertexBuffer, &offset);
	cmdBuffer->cmdDrawIndexedIndirect(drawData->indirectDrawBuffer.getBuffer(), 0, (uint32_t)drawData->terrain->getRegionCount(), sizeof(VkDrawIndexedIndirectCommand));
}

void ym::TerrainRenderer::recordFrustum(DrawData* drawData, uint32_t imageIndex, CommandBuffer* comdBuffer)
{
	comdBuffer->cmdBindPipeline(&this->frustumPipeline);
	std::vector<VkDescriptorSet> sets = { drawData->frustumDescriptorSet };
	std::vector<uint32_t> offsets;
	comdBuffer->cmdBindDescriptorSets(&this->frustumPipeline, 0, sets, offsets);
	comdBuffer->cmdDispatch((uint32_t)ceilf((float)drawData->terrain->getRegionCount() / 16), 1, 1);
}

void ym::TerrainRenderer::addTerrainToBatch(uint32_t imageIndex, DrawData& drawData)
{
	if (this->drawBatch.empty())
		this->drawBatch.resize(this->swapChain->getNumImages());

	uint64_t id = drawData.terrain->getUniqueID();
	auto& it = this->drawBatch[imageIndex].find(id);
	if (it == this->drawBatch[imageIndex].end())
	{
		// Create buffer for transformations.
		this->drawBatch[imageIndex][id] = drawData;
		this->drawBatch[imageIndex][id].exists = true;

		// Indirect draw data
		DrawData& data = this->drawBatch[imageIndex][id];
		initializeTerrain(data);

		// Tell renderer to recreate descriptors before rendering.
		this->shouldRecreateDescriptors[imageIndex] = true;
	}
	else
	{
		it->second.exists = true;
		it->second.terrain = drawData.terrain;
	}
}

void ym::TerrainRenderer::initializeTerrain(DrawData& drawData)
{
	// Create and populate indirect draw command buffer.
	createIndirectDrawBuffer(drawData);

	// Ubos for terrain transformation.
	drawData.terrainUbo.init(sizeof(TerrainData));
	drawData.terrainUbo.transfer(&drawData.transform, sizeof(TerrainData), 0);

	// Add initial data to the active buffer.
	drawData.terrain->fetchVertices(this->proximityWidth, drawData.terrain->getOrigin(), this->vertices);
	drawData.proximityVertices = this->activeVertexBuffer;

	// Ubo for frustum culling.
	drawData.proximityDataUBO.init(sizeof(ProximityData));
	ProximityData proxData = {};
	proxData.loadedWidth = this->proximityWidth;
	proxData.numIndicesPerReg = this->indiciesPerRegion;
	proxData.regionCount = drawData.terrain->getRegionCount();
	proxData.regWidth = this->regionSize;
	drawData.proximityDataUBO.transfer(&proxData, sizeof(ProximityData), 0);
}

void ym::TerrainRenderer::createBuffers()
{
	this->vertices.resize((size_t)this->proximityWidth * (size_t)this->proximityWidth);

	// Index buffer
	std::vector<uint32_t> queueIndices = { VulkanInstance::get()->getComputeQueue().queueIndex, VulkanInstance::get()->getGraphicsQueue().queueIndex, VulkanInstance::get()->getTransferQueue().queueIndex };
	uint32_t indexBufferSize = sizeof(unsigned) * getProximityIndiciesSize(this->regionSize, this->proximityWidth);
	this->indexBuffer.init(indexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queueIndices);
	this->memoryDeviceLocal.bindBuffer(&this->indexBuffer);

	// Vertex buffers
	queueIndices = { VulkanInstance::get()->getComputeQueue().queueIndex };
	this->vertexBuffer1.init(sizeof(Vertex) * this->vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queueIndices);
	this->vertexBuffer2.init(sizeof(Vertex) * this->vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queueIndices);
	this->memoryDeviceLocal.bindBuffer(&this->vertexBuffer1);
	this->memoryDeviceLocal.bindBuffer(&this->vertexBuffer2);
	this->memoryDeviceLocal.init(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Set inactive and active buffers.
	this->activeVertexBuffer = &this->vertexBuffer1;
	this->inactiveVertexBuffer = &this->vertexBuffer2;

	// Staging buffer for vertices and indices.
	uint64_t stagingBufferSize = glm::max(sizeof(Vertex) * this->vertices.size(), (uint64_t)indexBufferSize);
	this->stagingBuffer.init(stagingBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, { VulkanInstance::get()->getTransferQueue().queueIndex });
	this->memoryHost.bindBuffer(&this->stagingBuffer);
	this->memoryHost.init(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	// Transfer indices data to device.
	transferToDevice(&this->indexBuffer, &this->stagingBuffer, &this->memoryHost, this->indices.data(), this->indices.size());

	// Create UBOs for compute shader.
	this->frustumPlanesUBOs.resize(this->swapChain->getNumImages());
	for (UniformBuffer& ubo : this->frustumPlanesUBOs)
		ubo.init(sizeof(PlanesData));
}

void ym::TerrainRenderer::createDescriptorLayouts()
{
	// Frustum
	this->descriptorSetLayouts.frustum.add(new SSBO(VK_SHADER_STAGE_COMPUTE_BIT)); // Indirect command draw buffer.
	this->descriptorSetLayouts.frustum.add(new SSBO(VK_SHADER_STAGE_COMPUTE_BIT)); // Proximity vertices.
	this->descriptorSetLayouts.frustum.add(new UBO(VK_SHADER_STAGE_COMPUTE_BIT));  // Proximity data. 
	this->descriptorSetLayouts.frustum.add(new UBO(VK_SHADER_STAGE_COMPUTE_BIT));  // Frustum planes 
	this->descriptorSetLayouts.frustum.init();

	// Terrain
	this->descriptorSetLayouts.terrain.add(new UBO(VK_SHADER_STAGE_VERTEX_BIT)); // Transform.
	this->descriptorSetLayouts.terrain.init();

	// Material
	this->descriptorSetLayouts.material.add(new IMG(VK_SHADER_STAGE_FRAGMENT_BIT)); // BaseColor
	this->descriptorSetLayouts.material.add(new IMG(VK_SHADER_STAGE_FRAGMENT_BIT)); // MetallicRoughness
	this->descriptorSetLayouts.material.add(new IMG(VK_SHADER_STAGE_FRAGMENT_BIT)); // NormalTexture
	this->descriptorSetLayouts.material.add(new IMG(VK_SHADER_STAGE_FRAGMENT_BIT)); // OcclusionTexture
	this->descriptorSetLayouts.material.add(new IMG(VK_SHADER_STAGE_FRAGMENT_BIT)); // EmissiveTexture
	this->descriptorSetLayouts.material.init();
}

void ym::TerrainRenderer::recreateDescriptorPool(uint32_t imageIndex, uint32_t terrainCount)
{
	// Destroy the descriptor pool and its descriptor sets if it should be recreated.
	DescriptorPool& descriptorPool = this->descriptorPools[imageIndex];
	if (descriptorPool.wasCreated())
		descriptorPool.destroy();

	// There are many terrains with its own data => own descriptor set, multiply with the number of terrains.
	descriptorPool.addDescriptorLayout(this->descriptorSetLayouts.terrain, terrainCount);
	descriptorPool.addDescriptorLayout(this->descriptorSetLayouts.frustum, terrainCount);
	descriptorPool.addDescriptorLayout(this->descriptorSetLayouts.material, 1);

	descriptorPool.init(1);
}

void ym::TerrainRenderer::createDescriptorsSets(uint32_t imageIndex, std::map<uint64_t, DrawData>& drawBatch)
{
	VkDevice device = VulkanInstance::get()->getLogicalDevice();

	// Nodes, Materials and Models
	for (auto& drawData : drawBatch)
	{
		// Terrain
		{
			VkDescriptorSet& descriptorSet = drawData.second.terrain->getDescriptorSet();

			DescriptorSet terrainSet;
			terrainSet.init(this->descriptorSetLayouts.terrain, &descriptorSet, &this->descriptorPools[imageIndex]);
			terrainSet.setBufferDesc(0, drawData.second.terrainUbo.getDescriptor());
			terrainSet.update();
		}

		// Frustum
		{
			VkDescriptorSet& descriptorSet = drawData.second.frustumDescriptorSet;

			DescriptorSet frustumSet;
			frustumSet.init(this->descriptorSetLayouts.frustum, &descriptorSet, &this->descriptorPools[imageIndex]);
			frustumSet.setBufferDesc(0, drawData.second.indirectDrawBuffer.getBuffer(), 0, drawData.second.indirectDrawBuffer.getSize());
			frustumSet.setBufferDesc(1, drawData.second.proximityVertices->getBuffer(), 0, drawData.second.proximityVertices->getSize());
			frustumSet.setBufferDesc(2, drawData.second.proximityDataUBO.getDescriptor());
			frustumSet.setBufferDesc(3, this->frustumPlanesUBOs[imageIndex].getDescriptor());
			frustumSet.update();
		}

		// Materials
		/*std::vector<Material> & materials = drawData.second.model->materials;
		for (Material& material : materials)
		{
			if (material.descriptorSets.empty())
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
					for (uint32_t binding = 0; binding < 5; binding++)
						materialSet.setImageDesc(binding, imageInfos[binding]);
					materialSet.update();
				}
			}
		}*/
	}
}

uint32_t ym::TerrainRenderer::getProximityIndiciesSize(int32_t regionSize, int32_t proximityWidth)
{
	const uint32_t numQuads = regionSize - 1;
	uint32_t regionCount = static_cast<uint32_t>(ceilf((proximityWidth - 1) / (float)(regionSize - 1)));
	uint32_t size = regionCount * regionCount * numQuads * numQuads * 6;
	return size;
}

void ym::TerrainRenderer::generateIndicies(int32_t regionSize, int32_t proximityRadius)
{	
	const int numQuads = regionSize - 1;

	int regionCount = static_cast<int>(ceilf(((proximityRadius - 1) / (float)(regionSize - 1))));

	int vertOffsetX = 0;
	int vertOffsetZ = 0;

	int index = 0;
	uint32_t size = regionCount * regionCount * numQuads * numQuads * 6;
	this->indices.resize(size, 0);
	for (int j = 0; j < regionCount; j++)
	{
		for (int i = 0; i < regionCount; i++)
		{
			for (int z = vertOffsetZ; z < vertOffsetZ + numQuads; z++)
			{
				for (int x = vertOffsetX; x < vertOffsetX + numQuads; x++)
				{
					// First triangle
					this->indices[index++] = (z * proximityRadius + x);
					this->indices[index++] = ((z + 1) * proximityRadius + x);
					this->indices[index++] = ((z + 1) * proximityRadius + x + 1);
					// Second triangle
					this->indices[index++] = (z * proximityRadius + x);
					this->indices[index++] = ((z + 1) * proximityRadius + x + 1);
					this->indices[index++] = (z * proximityRadius + x + 1);
				}
			}
			vertOffsetX = (vertOffsetX + numQuads) % (proximityRadius - 1);
		}
		vertOffsetZ += numQuads;
	}
}

void ym::TerrainRenderer::createIndirectDrawBuffer(DrawData& drawData)
{
	uint32_t regionCount = (uint32_t)drawData.terrain->getRegionCount();

	drawData.indirectDrawBuffer.init(sizeof(VkDrawIndexedIndirectCommand) * regionCount, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		{ VulkanInstance::get()->getGraphicsQueue().queueIndex });
	drawData.memoryDeviceLocal.bindBuffer(&drawData.indirectDrawBuffer);
	drawData.memoryDeviceLocal.init(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	Buffer stagingBuffer;
	Memory stagingMemory;
	stagingBuffer.init(sizeof(VkDrawIndexedIndirectCommand) * regionCount, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, { VulkanInstance::get()->getTransferQueue().queueIndex });
	stagingMemory.bindBuffer(&stagingBuffer);
	stagingMemory.init(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	std::vector<VkDrawIndexedIndirectCommand> indirectData(regionCount);

	for (size_t i = 0; i < indirectData.size(); i++) {
		indirectData[i].instanceCount = 1;
		indirectData[i].firstInstance = i;
	}

	stagingMemory.directTransfer(&stagingBuffer, indirectData.data(), indirectData.size(), 0);

	CommandBuffer* cbuff = this->commandPoolGraphics.beginSingleTimeCommand();
	NameBinder::bindObject((uint64_t)cbuff->getCommandBuffer(), "Single Time Command Buffer for initial transfer of indirect command buffer [Terrain]");

	// Copy indirect command buffer data.
	VkBufferCopy region = {};
	region.srcOffset = 0;
	region.dstOffset = 0;
	region.size = indirectData.size();
	cbuff->cmdCopyBuffer(stagingBuffer.getBuffer(), drawData.indirectDrawBuffer.getBuffer(), 1, &region);

	cbuff->releaseBuffer(&drawData.indirectDrawBuffer, VK_ACCESS_TRANSFER_READ_BIT, VulkanInstance::get()->getGraphicsQueue().queueIndex, VulkanInstance::get()->getComputeQueue().queueIndex,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);

	this->commandPoolGraphics.endSingleTimeCommand(cbuff);

	stagingBuffer.destroy();
	stagingMemory.destroy();
}

void ym::TerrainRenderer::verticesToDevice(Buffer* device, std::vector<Vertex>& vertices)
{
	transferToDevice(device, &this->stagingBuffer, &this->memoryHost, vertices.data(), sizeof(Vertex) * vertices.size());
}

void ym::TerrainRenderer::transferToDevice(Buffer* buffer, Buffer* stagingBuffer, Memory* stagingMemory, void* data, uint64_t size)
{
	stagingMemory->directTransfer(stagingBuffer, data, size, 0);

	CommandBuffer* cbuff = this->commanPoolTransfer.beginSingleTimeCommand();
	NameBinder::bindObject((uint64_t)cbuff->getCommandBuffer(), "Single Time Command Buffer for function \"transferToDevice\" [Terrain]");

	// Copy vertex data.
	VkBufferCopy region = {};
	region.srcOffset = 0;
	region.dstOffset = 0;
	region.size = size;
	cbuff->cmdCopyBuffer(stagingBuffer->getBuffer(), buffer->getBuffer(), 1, &region);

	this->commanPoolTransfer.endSingleTimeCommand(cbuff);
}

void ym::TerrainRenderer::swapActiveBuffers()
{
	if (this->activeVertexBuffer == &this->vertexBuffer1)
	{
		this->activeVertexBuffer = &this->vertexBuffer2;
		this->inactiveVertexBuffer = &this->vertexBuffer1;
	}
	else
	{
		this->activeVertexBuffer = &this->vertexBuffer1;
		this->inactiveVertexBuffer = &this->vertexBuffer2;
	}
}

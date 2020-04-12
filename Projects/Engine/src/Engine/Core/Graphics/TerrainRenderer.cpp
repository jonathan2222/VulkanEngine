#include "stdafx.h"
#include "TerrainRenderer.h"

#include "Engine/Core/Threading/ThreadManager.h"

ym::TerrainRenderer::TerrainRenderer()
{
	this->threadID = 0;
	this->swapChain = nullptr;
}

ym::TerrainRenderer::~TerrainRenderer()
{
}

void ym::TerrainRenderer::init(SwapChain* swapChain, uint32_t threadID, RenderPass* renderPass)
{
	this->swapChain = swapChain;
	this->threadID = threadID;
	this->commandPool.init(CommandPool::Queue::GRAPHICS, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	this->commandBuffers = this->commandPool.createCommandBuffers(this->swapChain->getNumImages(), VK_COMMAND_BUFFER_LEVEL_SECONDARY);
}

void ym::TerrainRenderer::destroy()
{
	this->commandPool.destroy();
}

void ym::TerrainRenderer::setCamera(Camera* camera)
{
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
		this->drawBatch[imageIndex].clear();
}

void ym::TerrainRenderer::drawTerrain(uint32_t imageIndex, Terrain* terrain, const glm::mat4& transform)
{
	DrawData drawData;
	drawData.exists = true;
	drawData.terrain = terrain;
	addTerrainToBatch(imageIndex, drawData);
}

void ym::TerrainRenderer::end(uint32_t imageIndex)
{
	// Start recording all draw commands on the thread.
	CommandBuffer* currentBuffer = this->commandBuffers[imageIndex];
	currentBuffer->begin(VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, &inheritanceInfo);
	if (this->drawBatch.empty() == false)
	{
		for (auto& drawData : this->drawBatch[imageIndex])
		{
			if (drawData.second.exists)
			{
				ThreadManager::addWork(this->threadID, [=]() {
					recordTerrain(drawData.second.terrain, imageIndex, currentBuffer, inheritanceInfo);
				});
			}
		}

		ThreadManager::wait(this->threadID);
	};
	currentBuffer->end();
}

std::vector<ym::CommandBuffer*>& ym::TerrainRenderer::getBuffers()
{
	return this->commandBuffers;
}

void ym::TerrainRenderer::recordTerrain(Terrain* terrain, uint32_t imageIndex, CommandBuffer* cmdBuffer, VkCommandBufferInheritanceInfo inheritanceInfo)
{
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

		// Tell renderer to recreate descriptors before rendering.
		this->shouldRecreateDescriptors[imageIndex] = true;
	}
	else
	{
		it->second.exists = true;
		it->second.terrain = drawData.terrain;
	}
}

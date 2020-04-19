#include "stdafx.h"
#include "ThreadData.h"

void ym::ThreadData::init(uint32_t threadIndex, SwapChain* swapChain)
{
	this->threadIndex = threadIndex;
	this->graphicsPool.init(CommandPool::Queue::GRAPHICS, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	this->computePool.init(CommandPool::Queue::COMPUTE, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	this->transferPool.init(CommandPool::Queue::TRANSFER, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	this->secondaryCommandBuffersGraphics = this->graphicsPool.createCommandBuffers(swapChain->getNumImages(), VK_COMMAND_BUFFER_LEVEL_SECONDARY);
	this->secondaryCommandBuffersCompute = this->computePool.createCommandBuffers(swapChain->getNumImages(), VK_COMMAND_BUFFER_LEVEL_SECONDARY);
	this->secondaryCommandBuffersTransfer = this->transferPool.createCommandBuffers(swapChain->getNumImages(), VK_COMMAND_BUFFER_LEVEL_SECONDARY);
}

void ym::ThreadData::destory()
{
	this->graphicsPool.destroy();
	this->computePool.destroy();
	this->transferPool.destroy();
}

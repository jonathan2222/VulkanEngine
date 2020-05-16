#include "stdafx.h"
#include "CubeMap.h"

#include "Utils/stb_image.h"
#include "Engine/Core/Vulkan/Factory.h"
#include "Engine/Core/Vulkan/CommandBuffer.h"
#include "Engine/Core/Vulkan/CommandPool.h"
#include "Engine/Core/Application/LayerManager.h"

ym::CubeMap::CubeMap() : uniqueId(0), cubemapTexture(nullptr)
{
	static uint64_t generator = 0;
	this->uniqueId = ++generator;
}

ym::CubeMap::~CubeMap()
{
}

void ym::CubeMap::init(float scale, const std::string& texturePath)
{
	CommandPool* pool = &LayerManager::get()->getCommandPools()->graphicsPool;

	for (int i = 0; i < 36; i++)
		this->cube[i] *= scale;

	std::vector<std::string> facesNames = {
		"right.jpg",
		"left.jpg",
		"top.jpg",
		"bottom.jpg",
		"front.jpg",
		"back.jpg"
	};

	// Fetch each face of the cubemap.
	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
	int32_t width = 0, height = 0;
	std::vector<stbi_uc> facesData;
	std::vector<stbi_uc*> imgs;
	YM_LOG_INFO("Loading cubemap: {0}", texturePath.c_str());
	for (std::string& face : facesNames)
	{
		std::string finalFilePath = texturePath;
		finalFilePath += face;

		int channels;
		stbi_uc* img = stbi_load(finalFilePath.c_str(), &width, &height, &channels, 4);
		uint32_t size = width * height * 4;
		facesData.insert(facesData.end(), &img[0], &img[size]);
		imgs.push_back(img);
		YM_LOG_INFO(" ->Loaded face: {0}", face.c_str());
	}

	// Create cube map texture.
	{
		TextureDesc textureDesc;
		textureDesc.width = width;
		textureDesc.height = height;
		textureDesc.format = format;
		textureDesc.data = (void*)facesData.data();
		this->cubemapTexture = Factory::createCubeMapTexture(textureDesc, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_QUEUE_GRAPHICS_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
		Factory::transferCubeMapData(this->cubemapTexture, pool);
		for (stbi_uc* img : imgs) delete img;
		imgs.clear();
	}

	// Stage and transfer cube to device
	{
		Buffer stagingBuffer;
		Memory stagingMemory;
		stagingBuffer.init(sizeof(this->cube), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, { VulkanInstance::get()->getTransferQueue().queueIndex });
		stagingMemory.bindBuffer(&stagingBuffer);
		stagingMemory.init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		stagingMemory.directTransfer(&stagingBuffer, this->cube, sizeof(this->cube), 0);

		this->cubeBuffer.init(sizeof(this->cube), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, { VulkanInstance::get()->getGraphicsQueue().queueIndex, VulkanInstance::get()->getTransferQueue().queueIndex });
		this->cubeMemory.bindBuffer(&this->cubeBuffer);
		this->cubeMemory.init(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		CommandBuffer * cmdBuff = pool->beginSingleTimeCommand();

		VkBufferCopy region = {};
		region.srcOffset = 0;
		region.dstOffset = 0;
		region.size = sizeof(this->cube);
		cmdBuff->cmdCopyBuffer(stagingBuffer.getBuffer(), this->cubeBuffer.getBuffer(), 1, &region);

		pool->endSingleTimeCommand(cmdBuff);
		stagingBuffer.destroy();
		stagingMemory.destroy();
	}

	// Create sampler
	// TODO: This needs more control, should be compareOp=VK_COMPARE_OP_NEVER!
	this->cubemapSampler.init(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
}

void ym::CubeMap::init(float scale)
{
	CommandPool* pool = &LayerManager::get()->getCommandPools()->graphicsPool;

	for (int i = 0; i < 36; i++)
		this->cube[i] *= scale;

	// Stage and transfer cube to device
	{
		Buffer stagingBuffer;
		Memory stagingMemory;
		stagingBuffer.init(sizeof(this->cube), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, { VulkanInstance::get()->getTransferQueue().queueIndex });
		stagingMemory.bindBuffer(&stagingBuffer);
		stagingMemory.init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		stagingMemory.directTransfer(&stagingBuffer, this->cube, sizeof(this->cube), 0);

		this->cubeBuffer.init(sizeof(this->cube), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, { VulkanInstance::get()->getGraphicsQueue().queueIndex, VulkanInstance::get()->getTransferQueue().queueIndex });
		this->cubeMemory.bindBuffer(&this->cubeBuffer);
		this->cubeMemory.init(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		CommandBuffer * cmdBuff = pool->beginSingleTimeCommand();

		VkBufferCopy region = {};
		region.srcOffset = 0;
		region.dstOffset = 0;
		region.size = sizeof(this->cube);
		cmdBuff->cmdCopyBuffer(stagingBuffer.getBuffer(), this->cubeBuffer.getBuffer(), 1, &region);

		pool->endSingleTimeCommand(cmdBuff);
		stagingBuffer.destroy();
		stagingMemory.destroy();
	}

	// Create sampler
	// TODO: This needs more control, should be compareOp=VK_COMPARE_OP_NEVER!
	this->cubemapSampler.init(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
}

void ym::CubeMap::destroy()
{
	if (this->cubemapTexture != nullptr)
	{
		this->cubemapTexture->destroy();
		SAFE_DELETE(this->cubemapTexture);
	}
	this->cubemapSampler.destroy();

	this->cubeBuffer.destroy();
	this->cubeMemory.destroy();
}

void ym::CubeMap::setTexture(Texture* texturePtr)
{
	this->cubemapTexture = texturePtr;
}

ym::Texture* ym::CubeMap::getTexture()
{
	return this->cubemapTexture;
}

ym::Sampler* ym::CubeMap::getSampler()
{
	return &this->cubemapSampler;
}

ym::Buffer* ym::CubeMap::getVertexBuffer()
{
	return &this->cubeBuffer;
}

uint64_t ym::CubeMap::getUniqueId() const
{
	return this->uniqueId;
}

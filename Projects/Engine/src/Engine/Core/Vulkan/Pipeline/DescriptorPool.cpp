#include "stdafx.h"
#include "DescriptorPool.h"
#include "../VulkanInstance.h"

ym::DescriptorPool::DescriptorPool()
{
	this->pool = VK_NULL_HANDLE;
}

ym::DescriptorPool::~DescriptorPool()
{
	destroy();
}

void ym::DescriptorPool::addDescriptorLayout(DescriptorLayout descriptorSetLayout, uint32_t count)
{
	LayoutData layoutData;
	layoutData.layout = descriptorSetLayout;
	layoutData.count = count;
	this->descriptorSetLayouts.push_back(layoutData);
}

void ym::DescriptorPool::init(uint32_t numCopies)
{
	// Function to fill poolSizesMap with new data. (Accumulative)
	std::unordered_map<VkDescriptorType, uint32_t> poolSizesMap;
	auto addToMap = [&](std::vector<VkDescriptorPoolSize> & poolSizes) {
		for (VkDescriptorPoolSize& s : poolSizes)
		{
			auto& it = poolSizesMap.find(s.type);
			if (it == poolSizesMap.end()) poolSizesMap[s.type] = s.descriptorCount;
			else it->second += s.descriptorCount;
		}
	};

	uint32_t numSets = 0;
	for (LayoutData& layoutData : this->descriptorSetLayouts)
	{
		numSets += layoutData.count;
		addToMap(layoutData.layout.getPoolSizes(layoutData.count));
	}

	std::vector<VkDescriptorPoolSize> poolSizes;
	for (auto m : poolSizesMap)
		poolSizes.push_back({ m.first, m.second * numCopies });

	// Create descriptor pool.
	VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	descriptorPoolInfo.pPoolSizes = poolSizes.data();
	descriptorPoolInfo.maxSets = numSets * numCopies;
	VULKAN_CHECK(vkCreateDescriptorPool(VulkanInstance::get()->getLogicalDevice(), &descriptorPoolInfo, nullptr, &this->pool), "Could not create descriptor pool!");
}

void ym::DescriptorPool::destroy()
{
	if (this->pool != VK_NULL_HANDLE)
	{
		this->descriptorSetLayouts.clear();
		vkDestroyDescriptorPool(VulkanInstance::get()->getLogicalDevice(), this->pool, nullptr);
		this->pool = VK_NULL_HANDLE;
	}
}

bool ym::DescriptorPool::wasCreated() const
{
	return this->pool != VK_NULL_HANDLE;
}

VkDescriptorPool ym::DescriptorPool::getPool() const
{
	return this->pool;
}

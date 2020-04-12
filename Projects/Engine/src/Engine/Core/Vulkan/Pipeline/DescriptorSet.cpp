#include "stdafx.h"
#include "DescriptorSet.h"

#include "Engine/Core/Vulkan/VulkanInstance.h"

void ym::DescriptorSet::init(DescriptorLayout& layout, VkDescriptorSet* descriptorSetOut, DescriptorPool* descriptorPool)
{
	this->layout = layout;
	VkDescriptorSetLayout descriptorSetLayout = layout.getLayout();
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool->getPool();
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout;

	VULKAN_CHECK(vkAllocateDescriptorSets(VulkanInstance::get()->getLogicalDevice(), &allocInfo, descriptorSetOut), "Failed to create descriptor set!");
	this->descriptorSet = descriptorSetOut;
}

void ym::DescriptorSet::setBufferDesc(uint32_t binding, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
{
	VkDescriptorBufferInfo bufferInfo;
	bufferInfo.buffer = buffer;
	bufferInfo.offset = offset;
	bufferInfo.range = range;

	setBufferDesc(binding, bufferInfo);
}

void ym::DescriptorSet::setImageDesc(uint32_t binding, VkImageLayout layout, VkImageView view, VkSampler sampler)
{
	VkDescriptorImageInfo imageInfo;
	imageInfo.imageLayout = layout;
	imageInfo.imageView = view;
	imageInfo.sampler = sampler;

	setImageDesc(binding, imageInfo);
}

void ym::DescriptorSet::setBufferDesc(uint32_t binding, VkDescriptorBufferInfo info)
{
	std::pair<VkDescriptorType, GroupIndex> elem = this->layout.getWriteElem(binding);

	BufferGroup& group = this->bufferGroups[elem.second];
	if (group.infos.empty())
	{
		group.binding = binding;
		group.type = elem.first;
	}
	group.infos.push_back(info);
}

void ym::DescriptorSet::setImageDesc(uint32_t binding, VkDescriptorImageInfo info)
{
	std::pair<VkDescriptorType, GroupIndex> elem = this->layout.getWriteElem(binding);

	ImageGroup& group = this->imageGroups[elem.second];
	if (group.infos.empty())
	{
		group.binding = binding;
		group.type = elem.first;
	}
	group.infos.push_back(info);
}

void ym::DescriptorSet::update()
{
	std::vector<VkWriteDescriptorSet> descriptorWrites = {};

	for (auto& group : this->bufferGroups)
	{
		BufferGroup& bufferGroup = group.second;
		VkWriteDescriptorSet desc = {};
		desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		desc.dstSet = *this->descriptorSet;
		desc.dstArrayElement = 0;
		desc.descriptorType = bufferGroup.type;
		desc.pImageInfo = nullptr;
		desc.dstBinding = bufferGroup.binding;
		desc.descriptorCount = static_cast<uint32_t>(bufferGroup.infos.size());
		desc.pBufferInfo = bufferGroup.infos.data();
		desc.pTexelBufferView = nullptr;
		descriptorWrites.push_back(desc);
	}

	for (auto& group : this->imageGroups)
	{
		ImageGroup& imageGroup = group.second;
		VkWriteDescriptorSet desc = {};
		desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		desc.dstSet = *this->descriptorSet;
		desc.dstArrayElement = 0;
		desc.descriptorType = imageGroup.type;
		desc.pImageInfo = imageGroup.infos.data();
		desc.dstBinding = imageGroup.binding;
		desc.descriptorCount = static_cast<uint32_t>(imageGroup.infos.size());
		desc.pBufferInfo = nullptr;
		desc.pTexelBufferView = nullptr;
		descriptorWrites.push_back(desc);
	}

	vkUpdateDescriptorSets(VulkanInstance::get()->getLogicalDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	this->bufferGroups.clear();
	this->imageGroups.clear();
}

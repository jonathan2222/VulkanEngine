#pragma once

#include "stdafx.h"
#include "DescriptorPool.h"

namespace ym
{
	class DescriptorSet
	{
	public:
		void init(DescriptorLayout& layout, VkDescriptorSet* descriptorSetOut, DescriptorPool* descriptorPool);

		void setBufferDesc(uint32_t binding, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);
		void setImageDesc(uint32_t binding, VkImageLayout layout, VkImageView view, VkSampler sampler);
		void setBufferDesc(uint32_t binding, VkDescriptorBufferInfo info);
		void setImageDesc(uint32_t binding, VkDescriptorImageInfo info);
		void update();

	private:
		struct BufferGroup
		{
			uint32_t binding;
			VkDescriptorType type;
			std::vector<VkDescriptorBufferInfo> infos;
		};

		struct ImageGroup
		{
			uint32_t binding;
			VkDescriptorType type;
			std::vector<VkDescriptorImageInfo> infos;
		};

	private:
		std::unordered_map<GroupIndex, BufferGroup> bufferGroups;
		std::unordered_map<GroupIndex, ImageGroup> imageGroups;

		DescriptorLayout layout;
		VkDescriptorSet* descriptorSet{nullptr};
	};
}
#pragma once

#include <vector>

#include "DescriptorLayout.h"

typedef uint32_t SetIndex;

namespace ym
{
	class DescriptorManager
	{
	public:
		DescriptorManager();
		~DescriptorManager();

		void addLayout(const DescriptorLayout& layout);
		void init(uint32_t numCopies); // Create pool from layouts
		void destroy();

		// Cannot update buffers for different copies at the same time!
		void updateBufferDesc(SetIndex index, uint32_t binding, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);
		void updateImageDesc(SetIndex index, uint32_t binding, VkImageLayout layout, VkImageView view, VkSampler sampler);
		void updateSets(std::vector<SetIndex> sets, uint32_t copyIndex);

		VkDescriptorSet getSet(uint32_t copyIndex, SetIndex index) const;
		std::vector<DescriptorLayout> getLayouts() const;

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

		VkDescriptorPool pool;
		std::vector<DescriptorLayout> setLayouts;
		std::unordered_map<SetIndex, std::vector<VkDescriptorSet>> descriptorSets;
		std::unordered_map<SetIndex, std::unordered_map<GroupIndex, BufferGroup>> bufferGroups;
		std::unordered_map<SetIndex, std::unordered_map<GroupIndex, ImageGroup>> imageGroups;
	};
}
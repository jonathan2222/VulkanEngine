#pragma once

#include "stdafx.h"
#include "Descriptors.h"

typedef uint32_t GroupIndex;

namespace ym
{
	class DescriptorLayout
	{
	public:
		DescriptorLayout();
		~DescriptorLayout();

		void add(VkDescriptorSetLayoutBinding* descriptor);

		void init();
		void destroy();

		VkDescriptorSetLayout getLayout() const;
		std::vector<VkDescriptorPoolSize> getPoolSizes(uint32_t factor) const;
		std::pair<VkDescriptorType, GroupIndex> getWriteElem(uint32_t binding) const;

	private:
		std::vector<VkDescriptorSetLayoutBinding> descriptors;
		std::vector<std::vector<VkDescriptorSetLayoutBinding>> writeGroups;
		uint32_t currentBinding;
		VkDescriptorSetLayout layout;
	};
}
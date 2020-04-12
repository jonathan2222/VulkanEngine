#pragma once

#include "stdafx.h"
#include "DescriptorLayout.h"

namespace ym
{
	class DescriptorPool
	{
	public:
		DescriptorPool();
		~DescriptorPool();

		void addDescriptorLayout(DescriptorLayout descriptorSetLayout, uint32_t count);

		void init(uint32_t numCopies);
		void destroy();

		bool wasCreated() const;

		VkDescriptorPool getPool() const;

	private:
		struct LayoutData
		{
			DescriptorLayout layout;
			uint32_t count;
		};

		std::vector<LayoutData> descriptorSetLayouts;
		VkDescriptorPool pool;
	};
}
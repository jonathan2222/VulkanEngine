#pragma once

#include "stdafx.h"

namespace ym
{
	class PushConstants
	{
	public:
		PushConstants();
		~PushConstants();

		void init();
		void destroy();

		void addLayout(VkShaderStageFlags stageFlags, uint32_t size, uint32_t offset);

		// Set pointer to the data. This is the data which will be pushed to the command buffer.
		void setDataPtr(uint32_t size, uint32_t offset, const void* data);

		// Set pointer to the data. This uses the same offset and size when init was called.
		void setDataPtr(const void* data);

		std::vector<VkPushConstantRange> getRanges() const;
		const std::unordered_map<VkShaderStageFlags, VkPushConstantRange>& getRangeMap() const;
		const void* getData() const;
		uint32_t getSize() const;

	private:
		std::unordered_map<VkShaderStageFlags, VkPushConstantRange> ranges;
		char* data{ nullptr };
		uint32_t size{ 0 };
		uint32_t offset{ 0 };
	};
}
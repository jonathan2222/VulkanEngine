#pragma once

#include "glm/glm.hpp"
#include "Engine/Core/Vulkan/Pipeline/DescriptorLayout.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace ym
{
	struct SceneData
	{
		glm::mat4 proj;
		glm::mat4 view;
		alignas(16) glm::vec3 cPos;
	};

	struct SceneDescriptors
	{
		DescriptorLayout layout; // Holds camera data and other global data for the scene.
		std::vector<VkDescriptorSet> sets;

		// Holds descriptors for the environment maps.
		DescriptorLayout layoutEnv;
		std::vector<VkDescriptorSet> setsEnv;
	};
}
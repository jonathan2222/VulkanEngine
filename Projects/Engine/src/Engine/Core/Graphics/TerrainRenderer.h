#pragma once

#include "Engine/Core/Vulkan/VulkanInstance.h"
#include "Engine/Core/Vulkan/SwapChain.h"
#include "Engine/Core/Scene/Terrain/Terrain.h"
#include "Engine/Core/Vulkan/CommandBuffer.h"
#include "Engine/Core/Vulkan/CommandPool.h"
#include "Engine/Core/Vulkan/Pipeline/Shader.h"
#include "Engine/Core/Vulkan/Pipeline/Pipeline.h"
#include "Engine/Core/Camera.h"
#include "Engine/Core/Graphics/SceneData.h"

namespace ym
{
	class TerrainRenderer
	{
	public:
		TerrainRenderer();
		~TerrainRenderer();

		void init(SwapChain* swapChain, uint32_t threadID, RenderPass* renderPass, SceneDescriptors* sceneDescriptors);
		void destroy();

		/*
			Set the active camera which will be used when drawing.
		*/
		void setActiveCamera(Camera* camera);

		/*
			Prepare to draw.
		*/
		void begin(uint32_t imageIndex, VkCommandBufferInheritanceInfo inheritanceInfo);

		/*
			Draw the specified terrrain.
		*/
		void drawTerrain(uint32_t imageIndex, Terrain* terrain, const glm::mat4& transform);

		/*
			Gather and record draw commands.
		*/
		void end(uint32_t imageIndex);

		std::vector<CommandBuffer*>& getBuffers();

	private:
		struct SceneData
		{
			glm::mat4 proj;
			glm::mat4 view;
			alignas(16) glm::vec3 cPos;
		};

		struct DrawData
		{
			bool exists{ false }; // This is false when a terrain was removed. Will ensure that we do not recreate the descriptor pool if removing terrains only when adding.
			Terrain* terrain{ nullptr };
		};

		void recordTerrain(Terrain* terrain, uint32_t imageIndex, CommandBuffer* cmdBuffer, VkCommandBufferInheritanceInfo inheritanceInfo);

		void addTerrainToBatch(uint32_t imageIndex, DrawData& drawData);

	private:
		SwapChain* swapChain;
		VkCommandBufferInheritanceInfo inheritanceInfo;
		Camera* activeCamera{ nullptr };
		SceneDescriptors* sceneDescriptors{ nullptr };

		std::vector<bool> shouldRecreateDescriptors;
		std::vector<std::map<uint64_t, DrawData>> drawBatch;

		// Thread data
		std::vector<CommandBuffer*> commandBuffers;
		CommandPool commandPool;
		uint32_t threadID;
		Pipeline pipeline;
		Shader shader;
	};
}

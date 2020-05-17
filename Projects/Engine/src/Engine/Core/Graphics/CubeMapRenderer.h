#pragma once

#include "Engine/Core/Vulkan/VulkanInstance.h"
#include "Engine/Core/Vulkan/SwapChain.h"
#include "Engine/Core/Scene/Model/CubeMap.h"
#include "Engine/Core/Vulkan/CommandBuffer.h"
#include "Engine/Core/Vulkan/CommandPool.h"
#include "Engine/Core/Vulkan/Pipeline/Pipeline.h"
#include "Engine/Core/Vulkan/Pipeline/Shader.h"
#include "Engine/Core/Vulkan/Pipeline/DescriptorLayout.h"
#include "Engine/Core/Vulkan/Pipeline/DescriptorPool.h"
#include "Engine/Core/Vulkan/Buffers/UniformBuffer.h"
#include "Engine/Core/Vulkan/Buffers/StorageBuffer.h"
#include "Engine/Core/Camera.h"
#include "Engine/Core/Graphics/RenderInheritanceData.h"

namespace ym
{
	class CubeMapRenderer
	{
	public:
		CubeMapRenderer();
		virtual ~CubeMapRenderer();

		void init(SwapChain* swapChain, uint32_t threadID, RenderPass* renderPass, RenderInheritanceData* renderInheritanceData);
		void destroy();

		/*
			Convert a texture from equirectangular form to a cubemap form with the desired mip levels.
			If desiredMipLevels is 0, the final texture will use as many mip levels as it can.
		*/
		Texture* convertEquirectangularToCubemap(uint32_t sideSize, Texture* texture, CubeMap* cubeMap, uint32_t desiredMipLevels);

		/*
			Prepare to draw.
		*/
		void begin(uint32_t imageIndex, VkCommandBufferInheritanceInfo inheritanceInfo);

		/*
			Draw the specified cube map.
		*/
		void drawCubeMap(uint32_t imageIndex, CubeMap* model, const glm::mat4& transform);

		/*
			Draw instanced cube map.
		*/
		void drawCubeMap(uint32_t imageIndex, CubeMap* model, const std::vector<glm::mat4>& transforms);

		/*
			Gather and record draw commands.
		*/
		void end(uint32_t imageIndex);

		std::vector<CommandBuffer*>& getBuffers();

	private:
		struct DrawData
		{
			bool exists{ false }; // This is false when a model was removed. Will ensure that we do not recreate the descriptor pool if removing models only when adding.
			CubeMap* model{ nullptr };

			// Instance data
			std::vector<glm::mat4> transforms;
			StorageBuffer transformsBuffer;
			VkDescriptorSet descriptorSet;
		};

		void recordCubeMap(CubeMap* model, uint32_t imageIndex, VkDescriptorSet instanceDescriptorSet, uint32_t instanceCount, CommandBuffer* cmdBuffer);
		
		void createDescriptorLayouts();
		void recreateDescriptorPool(uint32_t imageIndex, uint32_t materialCount, uint32_t modelCount);
		void createDescriptorsSets(uint32_t imageIndex, std::map<uint64_t, DrawData>& drawBatch);

		uint64_t getUniqueId(uint32_t modelId, uint32_t instanceCount);
		void addModelToBatch(uint32_t imageIndex, DrawData& drawData);

	private:
		SwapChain* swapChain;

		std::vector<bool> shouldRecreateDescriptors;
		std::vector<std::map<uint64_t, DrawData>> drawBatch;
		VkCommandBufferInheritanceInfo inheritanceInfo;

		// Descriptors
		std::vector<DescriptorPool> descriptorPools;
		struct DescriptorSetLayouts
		{
			DescriptorLayout model;		// Holds instance transformations.
			DescriptorLayout material;	// Holds data of a specific material.
		} descriptorSetLayouts;
		RenderInheritanceData* renderInheritanceData;

		// Thread data
		std::vector<CommandBuffer*> commandBuffers;
		CommandPool commandPool;
		uint32_t threadID;
		Pipeline pipeline;
		Shader shader;
	};
}
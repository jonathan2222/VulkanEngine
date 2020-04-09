#pragma once

#include "Engine/Core/Vulkan/VulkanInstance.h"
#include "Engine/Core/Vulkan/SwapChain.h"
#include "Engine/Core/Scene/Model/Model.h"
#include "Engine/Core/Vulkan/CommandBuffer.h"
#include "Engine/Core/Vulkan/CommandPool.h"
#include "Engine/Core/Vulkan/Pipeline/Pipeline.h"
#include "Engine/Core/Vulkan/Pipeline/Shader.h"
#include "Engine/Core/Vulkan/Pipeline/DescriptorLayout.h"
#include "Engine/Core/Vulkan/Buffers/UniformBuffer.h"

namespace ym
{
	class ModelRenderer
	{
	public:
		ModelRenderer();
		virtual ~ModelRenderer();

		static ModelRenderer* get();

		void init(SwapChain* swapChain, uint32_t threadID, RenderPass* renderPass);
		void destroy();
		
		void begin(VkCommandBufferInheritanceInfo inheritanceInfo);

		/*
			Draw the specified model.
		*/
		void drawModel(Model* model, const glm::mat4& transform);

		/*
			Draw instanced model.
		*/
		void drawModel(Model* model, const std::vector<glm::mat4>& transforms);

		void end(uint32_t imageIndex);

		std::vector<CommandBuffer*>& getBuffers();

	private:
		struct SceneData
		{
			glm::mat4 proj;
			glm::mat4 view;
			glm::vec4 pos;
		};

		struct DrawData
		{
			Model* model{ nullptr };
			std::vector<glm::mat4> transforms; // Instance transform
		};

		void recordModel(Model* model, std::vector<glm::mat4> transform, uint32_t imageIndex, CommandBuffer* cmdBuffer, VkCommandBufferInheritanceInfo inheritanceInfo);
		void drawNode(uint32_t imageIndex, CommandBuffer* commandBuffer, Pipeline* pipeline, Model::Node& node, uint32_t instanceCount);
		
		void createBuffers();
		void createDescriptorLayouts();
		void recreateDescriptorPool(uint32_t materialCount, uint32_t nodeCount);
		void createDescriptorsSets(std::map<uint64_t, DrawData>& drawBatch);
		void createNodeDescriptorsSets(Model::Node& node);

	private:
		SwapChain* swapChain;

		uint64_t uniqueId;
		std::map<uint64_t, DrawData> drawBatch;
		VkCommandBufferInheritanceInfo inheritanceInfo;

		// Uniforms
		std::vector<UniformBuffer> sceneUBOs;

		// Descriptors
		VkDescriptorPool descriptorPool;
		struct DescriptorSetLayouts
		{
			DescriptorLayout scene;		// Holds camera data and other global data for the scene.
			DescriptorLayout node;		// Holds local node transformations. (This will be the same for every instance of the same model)
			DescriptorLayout material;	// Holds data of a specific material.
		} descriptorSetLayouts;
		struct DescriptorSet
		{
			VkDescriptorSet scene = VK_NULL_HANDLE;
		};
		std::vector<DescriptorSet> descriptorSets;

		// Thread data
		std::vector<CommandBuffer*> commandBuffers;
		CommandPool commandPool;
		uint32_t threadID;
		Pipeline pipeline;
		Shader shader;
	};
}
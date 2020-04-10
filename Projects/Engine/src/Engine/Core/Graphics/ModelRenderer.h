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
#include "Engine/Core/Vulkan/Buffers/StorageBuffer.h"
#include "Engine/Core/Camera.h"

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
		
		void setCamera(Camera* camera);

		void begin(uint32_t imageIndex, VkCommandBufferInheritanceInfo inheritanceInfo);

		/*
			Draw the specified model.
		*/
		void drawModel(uint32_t imageIndex, Model* model, const glm::mat4& transform);

		/*
			Draw instanced model.
		*/
		void drawModel(uint32_t imageIndex, Model* model, const std::vector<glm::mat4>& transforms);

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
			bool exists{false}; // This is false when a model was removed. Will ensure that we do not recreate the descriptor pool if removing models only when adding.
			Model* model{ nullptr };

			// Instance data
			std::vector<glm::mat4> transforms;
			StorageBuffer transformsBuffer;
			VkDescriptorSet descriptorSet;
		};

		void recordModel(Model* model, uint32_t imageIndex, VkDescriptorSet instanceDescriptorSet, uint32_t instanceCount, CommandBuffer* cmdBuffer, VkCommandBufferInheritanceInfo inheritanceInfo);
		void drawNode(uint32_t imageIndex, VkDescriptorSet instanceDescriptorSet, CommandBuffer* commandBuffer, Pipeline* pipeline, Model::Node& node, uint32_t instanceCount);
		
		void createBuffers();
		void createDescriptorLayouts();
		void recreateDescriptorPool(uint32_t imageIndex, uint32_t materialCount, uint32_t nodeCount, uint32_t modelCount);
		void createDescriptorsSets(uint32_t imageIndex, std::map<uint64_t, DrawData>& drawBatch);
		void createNodeDescriptorsSets(uint32_t imageIndex, Model::Node& node);

		uint64_t getUniqueId(uint32_t modelId, uint32_t instanceCount);
		void addModelToBatch(uint32_t imageIndex, DrawData& drawData);

	private:
		SwapChain* swapChain;

		std::vector<bool> shouldRecreateDescriptors;
		std::vector<std::map<uint64_t, DrawData>> drawBatch;
		VkCommandBufferInheritanceInfo inheritanceInfo;

		Camera* activeCamera{nullptr};

		// Uniforms
		std::vector<UniformBuffer> sceneUBOs;

		// Descriptors
		std::vector<VkDescriptorPool> descriptorPools;
		struct DescriptorSetLayouts
		{
			DescriptorLayout scene;		// Holds camera data and other global data for the scene.
			DescriptorLayout model;		// Holds instance transformations.
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
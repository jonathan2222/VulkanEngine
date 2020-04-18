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
#include "Engine/Core/Vulkan/Pipeline/DescriptorPool.h"
#include "Engine/Core/Vulkan/Buffers/Buffer.h"
#include "Engine/Core/Vulkan/Buffers/Memory.h"
#include "Engine/Core/Vulkan/Buffers/UniformBuffer.h"

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

		void preRecordGraphics(uint32_t imageIndex, CommandBuffer* commandBuffer);
		void postRecordGraphics(uint32_t imageIndex, CommandBuffer* commandBuffer);

		void preRecordCompute(uint32_t imageIndex, CommandBuffer* commandBuffer);
		void postRecordCompute(uint32_t imageIndex, CommandBuffer* commandBuffer);

		std::vector<CommandBuffer*>& getGraphicsBuffers();
		std::vector<CommandBuffer*>& getComputeBuffers();

	private:
		struct TerrainData
		{
			glm::mat4 transform;
		};

		struct ProximityData
		{
			uint32_t regWidth;           // Region width in number of vertices.
			uint32_t numIndicesPerReg;   // Number of indices per region.
			uint32_t loadedWidth;        // Loaded world width in verticies
			uint32_t regionCount;
		};

		struct PlanesData
		{
			Camera::Plane planes[6];  // Combination of normal (Pointing inwards) and position.
		};

		struct DrawData
		{
			bool exists{ false }; // This is false when a terrain was removed. Will ensure that we do not recreate the descriptor pool if removing terrains only when adding.
			Terrain* terrain{ nullptr };
			glm::mat4 transform;

			Buffer indirectDrawBuffer;
			Buffer* proximityVertices{nullptr};
			Memory memoryDeviceLocal;
			UniformBuffer terrainUbo;

			UniformBuffer proximityDataUBO;
			VkDescriptorSet frustumDescriptorSet;
		};

		void clearDrawBatch(uint32_t imageIndex);

		void recordTerrain(DrawData* drawData, uint32_t imageIndex, CommandBuffer* cmdBuffer);
		void recordFrustum(DrawData* drawData, uint32_t imageIndex, CommandBuffer* comdBuffer);

		void addTerrainToBatch(uint32_t imageIndex, DrawData& drawData);
		void initializeTerrain(DrawData& drawData);

		void createBuffers();

		void createDescriptorLayouts();
		void recreateDescriptorPool(uint32_t imageIndex, uint32_t terrainCount);
		void createDescriptorsSets(uint32_t imageIndex, std::map<uint64_t, DrawData>& drawBatch);

		uint32_t getProximityIndiciesSize(int32_t regionSize, int32_t proximityWidth);
		void generateIndicies(int32_t regionSize, int32_t proximityRadius);

		void createIndirectDrawBuffer(DrawData& drawData);
		void verticesToDevice(Buffer* device, std::vector<Vertex>& vertices);
		void transferToDevice(Buffer* buffer, Buffer* stagingBuffer, Memory* stagingMemory, void* data, uint64_t size);

		void swapActiveBuffers();

	private:
		SwapChain* swapChain;
		VkCommandBufferInheritanceInfo inheritanceInfo;
		Camera* activeCamera{ nullptr };
		SceneDescriptors* sceneDescriptors{ nullptr };

		std::vector<bool> shouldRecreateDescriptors;
		std::vector<std::map<uint64_t, DrawData>> drawBatch;

		// Terrain descriptors
		std::vector<DescriptorPool> descriptorPools;
		struct DescriptorSetLayouts
		{
			DescriptorLayout frustum;	// Holds indirect command buffer, vertices, frustum planes, and proximity data.
			DescriptorLayout terrain;   // Holds instance transformations.
			DescriptorLayout material;	// Holds data of a specific material.
		} descriptorSetLayouts;

		// Terrain data
		Buffer* activeVertexBuffer{nullptr};
		Buffer* inactiveVertexBuffer{ nullptr };
		Buffer vertexBuffer1;
		Buffer vertexBuffer2;
		Buffer indexBuffer;
		Memory memoryDeviceLocal;
		Memory memoryHost;
		Buffer stagingBuffer;
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		int32_t regionSize;
		int32_t proximityRadius;
		int32_t proximityWidth;
		int32_t indiciesPerRegion;
		std::vector<UniformBuffer> frustumPlanesUBOs;

		// Thread data
		std::vector<CommandBuffer*> commandBuffersGraphics;
		std::vector<CommandBuffer*> commandBuffersCompute;
		CommandPool commandPoolGraphics;
		CommandPool commandPoolCompute;
		CommandPool commanPoolTransfer;
		uint32_t threadID;
		Pipeline pipeline;
		Shader shader;
		Pipeline frustumPipeline;
		Shader frustumShader;
	};
}

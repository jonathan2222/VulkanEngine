#pragma once

#include "Engine/Core/Vulkan/VulkanInstance.h"
#include "Engine/Core/Vulkan/SwapChain.h"
#include "Engine/Core/Vulkan/Buffers/Framebuffer.h"
#include "Engine/Core/Graphics/ModelRenderer.h"
#include "Engine/Core/Graphics/CubeMapRenderer.h"
#include "Engine/Core/Graphics/TerrainRenderer.h"
#include "Engine/Core/Graphics/ThreadData.h"
#include "Engine/Core/Vulkan/Pipeline/RenderPass.h"
#include "Engine/Core/Camera.h"
#include "Engine/Core/Graphics/RenderInheritanceData.h"
#include "Engine/Core/Graphics/ImGUI/VKImgui.h"

namespace ym
{
	class ObjectManager;
	class Terrain;
	class Renderer
	{
	public:
		enum ERendererType
		{
			RENDER_TYPE_MODEL = 0,
			RENDER_TYPE_CUBE_MAP = 1,
			RENDER_TYPE_TERRAIN = 2,
			RENDER_TYPE_SIZE
		};

	public:
		Renderer();
		virtual ~Renderer();

		void init();
		void preDestroy();
		void destroy();

		void setActiveCamera(Camera* camera);

		Texture* getDefaultEnvironmentMap();

		/*
			Begin frame. Will return true if succeeded, false if the swap chain needs to be recreated.
		*/
		bool begin();
		
		/*
			Draw the specified model at the origin.
		*/
		void drawModel(Model* model);

		/*
			Draw the specified model.
		*/
		void drawModel(Model* model, const glm::mat4& transform);

		/*
			Draw instanced models.
		*/
		void drawModel(Model* model, const std::vector<glm::mat4>& transforms);

		/*
			Draw all objects in the object Manager as instanced models.
		*/
		void drawAllModels(ObjectManager* objectManager);

		/*
			Draw a skybox with the specifed cubemap texture.
		*/
		void drawSkybox(Texture* texture);

		/*
			Draw the scpecified cube map.
		*/
		void drawCubeMap(CubeMap* cubeMap, const glm::mat4& transform);

		/*
			Draw instanced cube maps.
		*/
		void drawCubeMap(CubeMap* cubeMap, const std::vector<glm::mat4>& transform);

		/*
			Draw a terrain object with frustum culling.
		*/
		void drawTerrain(Terrain* terrain, const glm::mat4& transform);

		/*
			End frame. Will return true if succeeded, false if the swap chain needs to be recreated.
		*/
		bool end();

	private:
		void createDepthTexture();
		void createRenderPass();
		void createFramebuffers(VkImageView depthAttachment);

		void createSyncObjects();
		void destroySyncObjects();

		void submit();

		void initInheritenceData();
		void destroyInheritanceData();
		void setupSceneDescriptors();

		void createDefaultEnvironmentTextures(const std::string& hdrImagePath);

	private:
		SwapChain swapChain;
		std::vector<Framebuffer> framebuffers;
		RenderPass renderPass;
		Texture* depthTexture;

		RenderInheritanceData renderInheritanceData;
		ModelRenderer modelRenderer;
		CubeMapRenderer cubeMapRenderer;
		//TerrainRenderer terrainRenderer;
		VKImgui imgui;

		// Scene data
		Camera* activeCamera{ nullptr };
		std::vector<UniformBuffer> sceneUBOs;
		DescriptorPool descriptorPool;

		// Environment map
		Texture* irradianceMap{ nullptr };
		Texture* prefilteredEnvironmentMap{ nullptr };
		Texture* environmentMap{ nullptr };
		Sampler irradianceSampler;
		Sampler prefilteredSampler;
		Sampler environmentSampler;

		// Recording
		std::vector<CommandBuffer*> primaryCommandBuffersGraphics;
		//std::vector<CommandBuffer*> primaryCommandBuffersCompute;
		VkCommandBufferInheritanceInfo inheritanceInfo;

		// Sync objects
		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		//std::vector<VkSemaphore> computeSemaphores;
		std::vector<VkFence> inFlightFences;
		std::vector<VkFence> imagesInFlight;
		uint32_t framesInFlight;
		uint32_t currentFrame;
		uint32_t imageIndex;
	};
}
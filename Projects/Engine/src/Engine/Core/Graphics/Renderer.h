#pragma once

#include "Engine/Core/Vulkan/VulkanInstance.h"
#include "Engine/Core/Vulkan/SwapChain.h"
#include "Engine/Core/Vulkan/Buffers/Framebuffer.h"
#include "Engine/Core/Graphics/ModelRenderer.h"
#include "Engine/Core/Vulkan/Pipeline/RenderPass.h"
#include "Engine/Core/Camera.h"

namespace ym
{
	class Renderer
	{
	public:
		Renderer();
		virtual ~Renderer();

		void init();
		void preDestroy();
		void destroy();

		void setCamera(Camera* camera);

		/*
			Begin frame. Will return true if succeeded, false if the swap chain needs to be recreated.
		*/
		bool begin();
		
		/*
			Draw the specified model.
		*/
		void drawModel(Model* model, const glm::mat4& transform);

		/*
			Draw instanced model.
		*/
		void drawModel(Model* model, const std::vector<glm::mat4>& transforms);

		/*
			End frame. Will return true if succeeded, false if the swap chain needs to be recreated.
		*/
		bool end();

	private:
		enum ERendererType
		{
			RENDER_TYPE_MODEL = 0,
			RENDER_TYPE_SIZE
		};

	private:
		void createDepthTexture();
		void createRenderPass();
		void createFramebuffers(VkImageView depthAttachment);

		void createSyncObjects();
		void destroySyncObjects();

		void submit();

	private:
		SwapChain swapChain;
		std::vector<Framebuffer> framebuffers;
		RenderPass renderPass;
		Texture* depthTexture;

		ModelRenderer modelRenderer;

		// Recording
		std::vector<CommandBuffer*> primaryCommandBuffers;
		VkCommandBufferInheritanceInfo inheritanceInfo;

		// Sync objects
		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence> inFlightFences;
		std::vector<VkFence> imagesInFlight;
		uint32_t framesInFlight;
		uint32_t currentFrame;
		uint32_t imageIndex;
	};
}
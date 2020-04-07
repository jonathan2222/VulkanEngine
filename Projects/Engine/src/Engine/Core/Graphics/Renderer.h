#pragma once

#include "Engine/Core/Vulkan/VulkanInstance.h"
#include "Engine/Core/Vulkan/SwapChain.h"
#include "Engine/Core/Vulkan/Buffers/Framebuffer.h"
#include "Engine/Core/Graphics/ModelRenderer.h"
#include "Engine/Core/Vulkan/Pipeline/RenderPass.h"

namespace ym
{
	class Renderer
	{
	public:
		Renderer();
		virtual ~Renderer();

		void init();
		void destroy();

	private:
		void createDepthTexture();
		void createRenderPass();
		void createFramebuffers(VkImageView depthAttachment);

	private:
		SwapChain swapChain;
		std::vector<Framebuffer> framebuffers;
		RenderPass renderPass;
		Texture* depthTexture;

		ModelRenderer modelRenderer;
	};
}
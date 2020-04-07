#pragma once

#include "Engine/Core/Vulkan/VulkanInstance.h"
#include "Engine/Core/Vulkan/SwapChain.h"
#include "Engine/Core/Scene/Model/Model.h"
#include "Engine/Core/Vulkan/CommandBuffer.h"
#include "Engine/Core/Vulkan/CommandPool.h"

namespace ym
{
	class ModelRenderer
	{
	public:
		ModelRenderer();
		virtual ~ModelRenderer();

		static ModelRenderer* get();

		void init(SwapChain* swapChain);
		void destroy();

		/*
			Begin frame. Will return true if succeeded, false if the swap chain needs to be recreated.
		*/
		bool begin();
		
		/*
			Draw the specified model.
		*/
		void drawModel(Model* model);
		
		/*
			End frame. Will return true if succeeded, false if the swap chain needs to be recreated.
		*/
		bool end();

	private:
		void createSyncObjects();
		void destroySyncObjects();

	private:
		SwapChain* swapChain;

		std::vector<CommandBuffer*> commandBuffers;
		CommandPool commandPool;

		// Sync objects
		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence> inFlightFences;
		std::vector<VkFence> imagesInFlight;
		uint32_t framesInFlight;
		uint32_t currentFrame;
		uint32_t imageIndex;
		uint32_t numImages;
	};
}
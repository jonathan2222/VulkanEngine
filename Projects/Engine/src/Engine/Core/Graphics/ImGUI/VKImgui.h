#pragma once

#include <vector>

#include "Utils/Imgui/imgui.h"

struct GLFWwindow;
namespace ym
{
	class RenderPass;
	class CommandPool;
	class CommandBuffer;
	class SwapChain;
	class Framebuffer;

	class VKImgui
	{
	public:
		VKImgui();
		~VKImgui();

		void init(SwapChain* swapChain);
		void destroy();

		void begin(uint32_t frameIndex, float dt);
		void end();
		void render();

		VkCommandBuffer getCurrentCommandBuffer() const;

	private:
		void createFramebuffers();
		void createCommandPoolAndBuffer();
		void createDescriptorPool();
		void initImgui();
		void createRenderPass();

		VkDescriptorPool descPool;
		SwapChain* swapChain;
		CommandPool* commandPool;
		std::vector<CommandBuffer*> commandBuffers;
		std::vector<Framebuffer*> framebuffers;
		uint32_t frameIndex;
		RenderPass* renderPass;
		GLFWwindow* window;
	};
}
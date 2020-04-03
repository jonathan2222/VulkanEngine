#pragma once

#include "stdafx.h"

namespace ym
{
	class RenderPass
	{
	public:
		struct SubpassInfo
		{
			int32_t depthStencilAttachmentIndex = -1;
			int32_t resolveAttachmentIndex = -1;
			std::vector<uint32_t> inputAttachmentIndices;
			std::vector<uint32_t> colorAttachmentIndices;
			std::vector<uint32_t> preserveAttachments; // Attachments which will not be used, but must be preserved through the subpass.
		};

	public:
		RenderPass();
		~RenderPass();

		void addColorAttachment(VkAttachmentDescription desc);
		void addDepthAttachment(VkAttachmentDescription desc);

		void addDefaultColorAttachment(VkFormat swapChainImageFormat);
		void addDefaultDepthAttachment();

		void addSubpass(SubpassInfo info);

		void addSubpassDependency(VkSubpassDependency dependency);
		void addDefaultSubpassDependency();

		void init();
		void destroy();

		VkRenderPass getRenderPass();

	private:
		struct SubpassInternal
		{
			std::vector<VkAttachmentReference> inputAttachments;
			std::vector<VkAttachmentReference> colorAttachments;
			std::vector<uint32_t> preserveAttachments;
			bool hasDepthStencilAttachment = false;
			VkAttachmentReference depthStencilAttachment = {};
			bool hasResolveAttachment = false;
			VkAttachmentReference resolveAttachment = {};
		};

		VkAttachmentReference getRef(uint32_t index);

		VkRenderPass renderPass;

		std::vector<SubpassInternal> subpasseInternals;
		std::vector<VkAttachmentDescription> attachments;
		std::vector<VkSubpassDependency> subpassDependencies;
		std::unordered_map<uint32_t, VkAttachmentReference> attachmentRefs;
	};
}

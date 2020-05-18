#include "stdafx.h"
#include "IBLFunctions.h"

#include "Engine/Core/Scene/Model/CubeMap.h"
#include "Engine/Core/Vulkan/Factory.h"
#include "Engine/Core/Vulkan/Sampler.h"
#include "Engine/Core/Vulkan/Pipeline/Shader.h"
#include "Engine/Core/Vulkan/Pipeline/Pipeline.h"
#include "Engine/Core/Vulkan/Pipeline/RenderPass.h"
#include "Engine/Core/Vulkan/Buffers/Framebuffer.h"
#include "Engine/Core/Vulkan/Pipeline/DescriptorLayout.h"
#include "Engine/Core/Vulkan/Pipeline/DescriptorPool.h"
#include "Engine/Core/Vulkan/Pipeline/DescriptorSet.h"

#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>

ym::Texture* ym::IBLFunctions::convertEquirectangularToCubemap(uint32_t sideSize, Texture* texture, uint32_t desiredMipLevels)
{
	CubeMap cubeModel;
	cubeModel.init(1.f);

	VkExtent2D extent = {};
	extent.width = sideSize;
	extent.height = sideSize;

	TextureDesc textureDesc;
	textureDesc.width = extent.width;
	textureDesc.height = extent.height;
	textureDesc.format = texture->textureDesc.format;
	textureDesc.data = nullptr;
	uint32_t mipLevels = desiredMipLevels == 0 ? static_cast<uint32_t>(std::floor(std::log2(std::max(textureDesc.width, textureDesc.height)))) + 1 : desiredMipLevels;
	Texture* newTexture = Factory::createCubeMapTexture(textureDesc, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_QUEUE_GRAPHICS_BIT, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);

	textureDesc = {};
	textureDesc.width = sideSize;
	textureDesc.height = sideSize;
	textureDesc.format = texture->textureDesc.format;
	textureDesc.data = nullptr;
	Texture* image = Factory::createTexture(textureDesc, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_QUEUE_GRAPHICS_BIT, VK_IMAGE_ASPECT_COLOR_BIT, 1);

	CommandPool* commandPool = &LayerManager::get()->getCommandPools()->graphicsPool;
	CommandBuffer* commandBuffer = commandPool->beginSingleTimeCommand();
	image->image.setLayout(
		commandBuffer,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	commandPool->endSingleTimeCommand(commandBuffer);

	Pipeline pipeline;
	Shader shader;
	shader.addStage(Shader::Type::VERTEX, YM_ASSETS_FILE_PATH + "Shaders/cubeVert.spv");
	shader.addStage(Shader::Type::FRAGMENT, YM_ASSETS_FILE_PATH + "Shaders/cubeConvertFrag.spv");
	shader.init();

	DescriptorPool descPool;
	DescriptorLayout descLayout;
	descLayout.add(new IMG(VK_SHADER_STAGE_FRAGMENT_BIT)); // Equirectangular texture
	descLayout.init();

	descPool.addDescriptorLayout(descLayout, 1);
	descPool.init(1);

	Sampler sampler;
	sampler.init(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, mipLevels);

	VkDescriptorSet descriptorSet;
	DescriptorSet modelSet;
	modelSet.init(descLayout, &descriptorSet, &descPool);
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = texture->image.getLayout();
	imageInfo.imageView = texture->imageView.getImageView();
	imageInfo.sampler = sampler.getSampler();
	modelSet.setImageDesc(0, imageInfo);
	modelSet.update();

	RenderPass renderPass;
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = texture->textureDesc.format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	renderPass.addColorAttachment(colorAttachment);

	// Use subpass dependencies for layout transitions
	VkSubpassDependency dependency1 = {};
	dependency1.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency1.dstSubpass = 0;
	dependency1.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependency1.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency1.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependency1.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency1.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	renderPass.addSubpassDependency(dependency1);
	VkSubpassDependency dependency2 = {};
	dependency2.srcSubpass = 0;
	dependency2.dstSubpass = VK_SUBPASS_EXTERNAL;
	dependency2.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency2.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependency2.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency2.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependency2.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	renderPass.addSubpassDependency(dependency2);

	RenderPass::SubpassInfo subpassInfo;
	subpassInfo.colorAttachmentIndices = { 0 };
	renderPass.addSubpass(subpassInfo);
	renderPass.init();

	std::vector<DescriptorLayout> descriptorLayouts = { descLayout };
	VkVertexInputBindingDescription vertexBindingDescription = {};
	vertexBindingDescription.binding = 0;
	vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	vertexBindingDescription.stride = sizeof(glm::vec4);
	VkVertexInputAttributeDescription vertexAttributeDescription = {};
	vertexAttributeDescription.binding = 0;
	vertexAttributeDescription.location = 0;
	vertexAttributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	vertexAttributeDescription.offset = 0;
	PipelineInfo pipelineInfo = {};
	pipelineInfo.vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	pipelineInfo.vertexInputInfo.vertexBindingDescriptionCount = 1;
	pipelineInfo.vertexInputInfo.vertexAttributeDescriptionCount = 1;
	pipelineInfo.vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDescription;
	pipelineInfo.vertexInputInfo.pVertexAttributeDescriptions = &vertexAttributeDescription;

	pipelineInfo.rasterizer = {};
	pipelineInfo.rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	pipelineInfo.rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	pipelineInfo.rasterizer.lineWidth = 1.0f;
	pipelineInfo.rasterizer.cullMode = VK_CULL_MODE_NONE;
	pipelineInfo.rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	pipelineInfo.depthStencil = {};
	pipelineInfo.depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	pipelineInfo.depthStencil.depthTestEnable = VK_FALSE;
	pipelineInfo.depthStencil.depthWriteEnable = VK_FALSE;
	pipelineInfo.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	pipelineInfo.depthStencil.depthBoundsTestEnable = VK_FALSE;
	pipelineInfo.depthStencil.minDepthBounds = 0.0f; // Optional
	pipelineInfo.depthStencil.maxDepthBounds = 1.0f; // Optional
	pipelineInfo.depthStencil.stencilTestEnable = VK_FALSE;
	pipelineInfo.depthStencil.front = {}; // Optional
	pipelineInfo.depthStencil.back = {}; // Optional

	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.size = sizeof(glm::mat4);
	pushConstantRange.offset = 0;
	pipeline.setPushConstant(pushConstantRange);
	pipeline.setPipelineInfo(PipelineInfoFlag::DEAPTH_STENCIL | PipelineInfoFlag::RASTERIZATION | PipelineInfoFlag::VERTEX_INPUT, pipelineInfo);
	pipeline.setDescriptorLayouts(descriptorLayouts);
	pipeline.setGraphicsPipelineInfo(extent, &renderPass);
	pipeline.init(Pipeline::Type::GRAPHICS, &shader);

	std::vector<VkClearValue> clearValues = {};
	VkClearValue value;
	value.color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues.push_back(value);

	VkFramebuffer frameBuffer;
	VkFramebufferCreateInfo frameBufferInfo = {};
	frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferInfo.renderPass = renderPass.getRenderPass();
	frameBufferInfo.width = extent.width;
	frameBufferInfo.height = extent.height;
	frameBufferInfo.attachmentCount = 1;
	VkImageView imageView = image->imageView.getImageView();
	frameBufferInfo.pAttachments = &imageView;
	frameBufferInfo.layers = 1;
	vkCreateFramebuffer(VulkanInstance::get()->getLogicalDevice(), &frameBufferInfo, nullptr, &frameBuffer);

	std::vector<glm::mat4> matrices = {
		// POSITIVE_X
		glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		// NEGATIVE_X
		glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		// POSITIVE_Y
		glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		// NEGATIVE_Y
		glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		// POSITIVE_Z
		glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		// NEGATIVE_Z
		glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
	};

	commandBuffer = commandPool->beginSingleTimeCommand();

	for (uint32_t i = 0; i < mipLevels; i++) {
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = i;
		subresourceRange.levelCount = 1;
		subresourceRange.layerCount = 6;
		newTexture->image.setLayout(
			commandBuffer,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			subresourceRange);

		for (uint32_t f = 0; f < 6; f++) {
			commandBuffer->cmdBeginRenderPass(&renderPass, frameBuffer, extent, clearValues, VK_SUBPASS_CONTENTS_INLINE);

			VkBuffer buffer = cubeModel.getVertexBuffer()->getBuffer();

			// Update shader push constant block
			glm::mat4 mvp = glm::perspective((float)(3.1415f / 2.0), 1.0f, 0.1f, (float)sideSize) * matrices[f];
			vkCmdPushConstants(commandBuffer->getCommandBuffer(), pipeline.getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &mvp);

			commandBuffer->cmdBindPipeline(&pipeline);
			VkDeviceSize offset = 0;
			commandBuffer->cmdBindVertexBuffers(0, 1, &buffer, &offset);

			std::vector<VkDescriptorSet> sets = { descriptorSet };
			std::vector<uint32_t> offsets;
			commandBuffer->cmdBindDescriptorSets(&pipeline, 0, sets, offsets);
			commandBuffer->cmdDraw(36, 1, 0, 0);

			commandBuffer->cmdEndRenderPass();

			image->image.setLayout(
				commandBuffer,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

			// Copy region for transfer from framebuffer to cube face
			VkImageCopy copyRegion = {};
			copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.srcSubresource.baseArrayLayer = 0;
			copyRegion.srcSubresource.mipLevel = 0;
			copyRegion.srcSubresource.layerCount = 1;
			copyRegion.srcOffset = { 0, 0, 0 };
			copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.dstSubresource.baseArrayLayer = f;
			copyRegion.dstSubresource.mipLevel = 0;
			copyRegion.dstSubresource.layerCount = 1;
			copyRegion.dstOffset = { 0, 0, 0 };
			copyRegion.extent.width = extent.width;
			copyRegion.extent.height = extent.height;
			copyRegion.extent.depth = 1;

			vkCmdCopyImage(
				commandBuffer->getCommandBuffer(),
				image->image.getImage(),
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				newTexture->image.getImage(),
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&copyRegion);

			// Transform framebuffer color attachment back (Used if more mipmaps else not necessary!)
			image->image.setLayout(
				commandBuffer,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		}

		if (mipLevels == 1)
		{
			newTexture->image.setLayout(
				commandBuffer,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				subresourceRange);
		}
	}

	commandPool->endSingleTimeCommand(commandBuffer);

	if (mipLevels > 1)
		Factory::generateMipmaps(newTexture);

	sampler.destroy();
	vkDestroyFramebuffer(VulkanInstance::get()->getLogicalDevice(), frameBuffer, nullptr);
	descLayout.destroy();
	descPool.destroy();
	renderPass.destroy();
	shader.destroy();
	pipeline.destroy();

	image->destroy();
	SAFE_DELETE(image);

	cubeModel.destroy();

	return newTexture;
}

std::pair<ym::Texture*, ym::Texture*> ym::IBLFunctions::createIrradianceAndPrefilteredMap(Texture* environmentCubeTexture)
{
	ym::Texture* irradianceTexture = nullptr;
	ym::Texture* prefilteredEnvTexture = nullptr;

	CubeMap cubeModel;
	cubeModel.init(1.f);
	enum Target { IRRADIANCE = 0, PREFILTEREDENV = 1 };
	for (uint32_t target = 0; target < PREFILTEREDENV + 1; target++) {

		uint32_t sideSize = 0;
		VkFormat format;
		switch (target)
		{
		case IRRADIANCE:
			format = VK_FORMAT_R32G32B32A32_SFLOAT;
			sideSize = 64;
			break;
		case PREFILTEREDENV:
			format = VK_FORMAT_R16G16B16A16_SFLOAT;
			sideSize = 512;
			break;
		}

		VkExtent2D extent = {};
		extent.width = sideSize;
		extent.height = sideSize;

		TextureDesc textureDesc;
		textureDesc.width = extent.width;
		textureDesc.height = extent.height;
		textureDesc.format = format;
		textureDesc.data = nullptr;
		uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(sideSize))) + 1;
		Texture* newTexture = Factory::createCubeMapTexture(textureDesc, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_QUEUE_GRAPHICS_BIT, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
		switch (target) {
		case IRRADIANCE:
			irradianceTexture = newTexture;
			break;
		case PREFILTEREDENV:
			prefilteredEnvTexture = newTexture;
			break;
		};

		textureDesc = {};
		textureDesc.width = sideSize;
		textureDesc.height = sideSize;
		textureDesc.format = format;
		textureDesc.data = nullptr;
		Texture* image = Factory::createTexture(textureDesc, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_QUEUE_GRAPHICS_BIT, VK_IMAGE_ASPECT_COLOR_BIT, 1);

		CommandPool* commandPool = &LayerManager::get()->getCommandPools()->graphicsPool;
		CommandBuffer* commandBuffer = commandPool->beginSingleTimeCommand();
		image->image.setLayout(
			commandBuffer,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		commandPool->endSingleTimeCommand(commandBuffer);

		Pipeline pipeline;
		Shader shader;
		shader.addStage(Shader::Type::VERTEX, YM_ASSETS_FILE_PATH + "Shaders/cubeVert.spv");
		switch (target)
		{
		case IRRADIANCE:
			shader.addStage(Shader::Type::FRAGMENT, YM_ASSETS_FILE_PATH + "Shaders/irradianceCubeFrag.spv");
			break;
		case PREFILTEREDENV:
			shader.addStage(Shader::Type::FRAGMENT, YM_ASSETS_FILE_PATH + "Shaders/prefilteredEnvMapFrag.spv");
			break;
		}
		shader.init();

		DescriptorPool descPool;
		DescriptorLayout descLayout;
		descLayout.add(new IMG(VK_SHADER_STAGE_FRAGMENT_BIT)); // Environment cube texture
		descLayout.init();
		descPool.addDescriptorLayout(descLayout, 1);
		descPool.init(1);

		Sampler sampler;
		sampler.init(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);

		VkDescriptorSet descriptorSet;
		DescriptorSet modelSet;
		modelSet.init(descLayout, &descriptorSet, &descPool);
		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = environmentCubeTexture->image.getLayout();
		imageInfo.imageView = environmentCubeTexture->imageView.getImageView();
		imageInfo.sampler = sampler.getSampler();
		modelSet.setImageDesc(0, imageInfo);
		modelSet.update();

		RenderPass renderPass;
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = format;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		renderPass.addColorAttachment(colorAttachment);

		// Use subpass dependencies for layout transitions
		VkSubpassDependency dependency1 = {};
		dependency1.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency1.dstSubpass = 0;
		dependency1.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependency1.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency1.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependency1.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependency1.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		renderPass.addSubpassDependency(dependency1);
		VkSubpassDependency dependency2 = {};
		dependency2.srcSubpass = 0;
		dependency2.dstSubpass = VK_SUBPASS_EXTERNAL;
		dependency2.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency2.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependency2.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependency2.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependency2.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		renderPass.addSubpassDependency(dependency2);

		RenderPass::SubpassInfo subpassInfo;
		subpassInfo.colorAttachmentIndices = { 0 };
		renderPass.addSubpass(subpassInfo);
		renderPass.init();

		std::vector<DescriptorLayout> descriptorLayouts = { descLayout };
		VkVertexInputBindingDescription vertexBindingDescription = {};
		vertexBindingDescription.binding = 0;
		vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		vertexBindingDescription.stride = sizeof(glm::vec4);
		VkVertexInputAttributeDescription vertexAttributeDescription = {};
		vertexAttributeDescription.binding = 0;
		vertexAttributeDescription.location = 0;
		vertexAttributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		vertexAttributeDescription.offset = 0;
		PipelineInfo pipelineInfo = {};
		pipelineInfo.vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		pipelineInfo.vertexInputInfo.vertexBindingDescriptionCount = 1;
		pipelineInfo.vertexInputInfo.vertexAttributeDescriptionCount = 1;
		pipelineInfo.vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDescription;
		pipelineInfo.vertexInputInfo.pVertexAttributeDescriptions = &vertexAttributeDescription;

		pipelineInfo.rasterizer = {};
		pipelineInfo.rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		pipelineInfo.rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		pipelineInfo.rasterizer.lineWidth = 1.0f;
		pipelineInfo.rasterizer.cullMode = VK_CULL_MODE_NONE;
		pipelineInfo.rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		pipelineInfo.depthStencil = {};
		pipelineInfo.depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		pipelineInfo.depthStencil.depthTestEnable = VK_FALSE;
		pipelineInfo.depthStencil.depthWriteEnable = VK_FALSE;
		pipelineInfo.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		pipelineInfo.depthStencil.depthBoundsTestEnable = VK_FALSE;
		pipelineInfo.depthStencil.minDepthBounds = 0.0f; // Optional
		pipelineInfo.depthStencil.maxDepthBounds = 1.0f; // Optional
		pipelineInfo.depthStencil.stencilTestEnable = VK_FALSE;
		pipelineInfo.depthStencil.front = {}; // Optional
		pipelineInfo.depthStencil.back = {}; // Optional

		struct PushBlockIrradiance {
			glm::mat4 mvp;
			float deltaPhi = (2.0f * glm::pi<float>()) / 180.0f;
			float deltaTheta = (0.5f * glm::pi<float>()) / 64.0f;
		} pushBlockIrradiance;

		struct PushBlockPrefilterEnv {
			glm::mat4 mvp;
			float roughness;
			uint32_t numSamples = 32u;
		} pushBlockPrefilterEnv;

		VkPushConstantRange pushConstantRange = {};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		switch (target) {
		case IRRADIANCE:
			pushConstantRange.size = sizeof(PushBlockIrradiance);
			break;
		case PREFILTEREDENV:
			pushConstantRange.size = sizeof(PushBlockPrefilterEnv);
			break;
		};
		pipeline.setPushConstant(pushConstantRange);
		pipeline.setPipelineInfo(PipelineInfoFlag::DEAPTH_STENCIL | PipelineInfoFlag::RASTERIZATION | PipelineInfoFlag::VERTEX_INPUT, pipelineInfo);
		pipeline.setDescriptorLayouts(descriptorLayouts);
		pipeline.setGraphicsPipelineInfo(extent, &renderPass);
		pipeline.init(Pipeline::Type::GRAPHICS, &shader);

		std::vector<VkClearValue> clearValues = {};
		VkClearValue value;
		value.color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues.push_back(value);

		VkFramebuffer frameBuffer;
		VkFramebufferCreateInfo frameBufferInfo = {};
		frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferInfo.renderPass = renderPass.getRenderPass();
		frameBufferInfo.width = extent.width;
		frameBufferInfo.height = extent.height;
		frameBufferInfo.attachmentCount = 1;
		VkImageView imageView = image->imageView.getImageView();
		frameBufferInfo.pAttachments = &imageView;
		frameBufferInfo.layers = 1;
		vkCreateFramebuffer(VulkanInstance::get()->getLogicalDevice(), &frameBufferInfo, nullptr, &frameBuffer);

		std::vector<glm::mat4> matrices = {
			// POSITIVE_X
			glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			// NEGATIVE_X
			glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			// POSITIVE_Y
			glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			// NEGATIVE_Y
			glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			// POSITIVE_Z
			glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			// NEGATIVE_Z
			glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		};

		commandBuffer = commandPool->beginSingleTimeCommand();

		for (uint32_t i = 0; i < mipLevels; i++) {
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = i;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = 6;
			newTexture->image.setLayout(
				commandBuffer,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				subresourceRange);

			for (uint32_t f = 0; f < 6; f++) {
				commandBuffer->cmdBeginRenderPass(&renderPass, frameBuffer, extent, clearValues, VK_SUBPASS_CONTENTS_INLINE);

				VkBuffer buffer = cubeModel.getVertexBuffer()->getBuffer();

				// Pass parameters for current pass using a push constant block
				float pi = glm::pi<float>();
				switch (target) {
				case IRRADIANCE:
					pushBlockIrradiance.mvp = glm::perspective((float)(pi / 2.0), 1.0f, 0.1f, 512.0f) * matrices[f];
					vkCmdPushConstants(commandBuffer->getCommandBuffer(), pipeline.getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushBlockIrradiance), &pushBlockIrradiance);
					break;
				case PREFILTEREDENV:
					pushBlockPrefilterEnv.mvp = glm::perspective((float)(pi / 2.0), 1.0f, 0.1f, 512.0f) * matrices[f];
					pushBlockPrefilterEnv.roughness = (float)i / (float)(mipLevels - 1);
					vkCmdPushConstants(commandBuffer->getCommandBuffer(), pipeline.getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushBlockPrefilterEnv), &pushBlockPrefilterEnv);
					break;
				};

				commandBuffer->cmdBindPipeline(&pipeline);
				VkDeviceSize offset = 0;
				commandBuffer->cmdBindVertexBuffers(0, 1, &buffer, &offset);

				std::vector<VkDescriptorSet> sets = { descriptorSet };
				std::vector<uint32_t> offsets;
				commandBuffer->cmdBindDescriptorSets(&pipeline, 0, sets, offsets);
				commandBuffer->cmdDraw(36, 1, 0, 0);

				commandBuffer->cmdEndRenderPass();

				image->image.setLayout(
					commandBuffer,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

				// Copy region for transfer from framebuffer to cube face
				VkImageCopy copyRegion = {};
				copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.srcSubresource.baseArrayLayer = 0;
				copyRegion.srcSubresource.mipLevel = 0;
				copyRegion.srcSubresource.layerCount = 1;
				copyRegion.srcOffset = { 0, 0, 0 };
				copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.dstSubresource.baseArrayLayer = f;
				copyRegion.dstSubresource.mipLevel = 0;
				copyRegion.dstSubresource.layerCount = 1;
				copyRegion.dstOffset = { 0, 0, 0 };
				copyRegion.extent.width = extent.width;
				copyRegion.extent.height = extent.height;
				copyRegion.extent.depth = 1;

				vkCmdCopyImage(
					commandBuffer->getCommandBuffer(),
					image->image.getImage(),
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					newTexture->image.getImage(),
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1,
					&copyRegion);

				// Transform framebuffer color attachment back (Used if more mipmaps else not necessary!)
				image->image.setLayout(
					commandBuffer,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			}

			if (mipLevels == 1)
			{
				newTexture->image.setLayout(
					commandBuffer,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					subresourceRange);
			}
		}

		commandPool->endSingleTimeCommand(commandBuffer);

		if (mipLevels > 1)
			Factory::generateMipmaps(newTexture);

		sampler.destroy();
		vkDestroyFramebuffer(VulkanInstance::get()->getLogicalDevice(), frameBuffer, nullptr);
		descLayout.destroy();
		descPool.destroy();
		renderPass.destroy();
		shader.destroy();
		pipeline.destroy();

		image->destroy();
		SAFE_DELETE(image);
	}

	cubeModel.destroy();

	return std::pair<Texture*, Texture*>(irradianceTexture, prefilteredEnvTexture);
}

ym::Texture* ym::IBLFunctions::createBRDFLutTexture(uint32_t size)
{
	VkFormat format = VK_FORMAT_R16G16_SFLOAT;
	VkExtent2D extent = {};
	extent.width = size;
	extent.height = size;

	TextureDesc textureDesc;
	textureDesc.width = extent.width;
	textureDesc.height = extent.height;
	textureDesc.format = format;
	textureDesc.data = nullptr;
	Texture* brdfLutTexture = Factory::createTexture(textureDesc, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_QUEUE_GRAPHICS_BIT, VK_IMAGE_ASPECT_COLOR_BIT, 1);

	CommandPool* commandPool = &LayerManager::get()->getCommandPools()->graphicsPool;
	CommandBuffer* commandBuffer = commandPool->beginSingleTimeCommand();
	brdfLutTexture->image.setLayout(
		commandBuffer,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	commandPool->endSingleTimeCommand(commandBuffer);

	Pipeline pipeline;
	Shader shader;
	shader.addStage(Shader::Type::VERTEX, YM_ASSETS_FILE_PATH + "Shaders/genbrdflutVert.spv");
	shader.addStage(Shader::Type::FRAGMENT, YM_ASSETS_FILE_PATH + "Shaders/genbrdflutFrag.spv");
	shader.init();

	// Create empty descriptor layout.
	DescriptorLayout descLayout;
	descLayout.init();

	Sampler sampler;
	sampler.init(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);

	RenderPass renderPass;
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	renderPass.addColorAttachment(colorAttachment);

	// Use subpass dependencies for layout transitions
	VkSubpassDependency dependency1 = {};
	dependency1.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency1.dstSubpass = 0;
	dependency1.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependency1.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency1.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependency1.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency1.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	renderPass.addSubpassDependency(dependency1);
	VkSubpassDependency dependency2 = {};
	dependency2.srcSubpass = 0;
	dependency2.dstSubpass = VK_SUBPASS_EXTERNAL;
	dependency2.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency2.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependency2.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency2.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependency2.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	renderPass.addSubpassDependency(dependency2);

	RenderPass::SubpassInfo subpassInfo;
	subpassInfo.colorAttachmentIndices = { 0 };
	renderPass.addSubpass(subpassInfo);
	renderPass.init();

	std::vector<DescriptorLayout> descriptorLayouts = { descLayout };
	PipelineInfo pipelineInfo = {};
	pipelineInfo.vertexInputInfo = {};
	pipelineInfo.vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	pipelineInfo.rasterizer = {};
	pipelineInfo.rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	pipelineInfo.rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	pipelineInfo.rasterizer.lineWidth = 1.0f;
	pipelineInfo.rasterizer.cullMode = VK_CULL_MODE_NONE;
	pipelineInfo.rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	pipelineInfo.depthStencil = {};
	pipelineInfo.depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	pipelineInfo.depthStencil.depthTestEnable = VK_FALSE;
	pipelineInfo.depthStencil.depthWriteEnable = VK_FALSE;
	pipelineInfo.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	pipelineInfo.depthStencil.depthBoundsTestEnable = VK_FALSE;
	pipelineInfo.depthStencil.minDepthBounds = 0.0f; // Optional
	pipelineInfo.depthStencil.maxDepthBounds = 1.0f; // Optional
	pipelineInfo.depthStencil.stencilTestEnable = VK_FALSE;
	pipelineInfo.depthStencil.front = {}; // Optional
	pipelineInfo.depthStencil.back = {}; // Optional

	pipeline.setPipelineInfo(PipelineInfoFlag::DEAPTH_STENCIL | PipelineInfoFlag::RASTERIZATION | PipelineInfoFlag::VERTEX_INPUT, pipelineInfo);
	pipeline.setDescriptorLayouts(descriptorLayouts);
	pipeline.setGraphicsPipelineInfo(extent, &renderPass);
	pipeline.init(Pipeline::Type::GRAPHICS, &shader);

	std::vector<VkClearValue> clearValues = {};
	VkClearValue value;
	value.color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues.push_back(value);

	VkFramebuffer frameBuffer;
	VkFramebufferCreateInfo frameBufferInfo = {};
	frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferInfo.renderPass = renderPass.getRenderPass();
	frameBufferInfo.width = extent.width;
	frameBufferInfo.height = extent.height;
	frameBufferInfo.attachmentCount = 1;
	VkImageView imageView = brdfLutTexture->imageView.getImageView();
	frameBufferInfo.pAttachments = &imageView;
	frameBufferInfo.layers = 1;
	vkCreateFramebuffer(VulkanInstance::get()->getLogicalDevice(), &frameBufferInfo, nullptr, &frameBuffer);

	commandBuffer = commandPool->beginSingleTimeCommand();
	commandBuffer->cmdBeginRenderPass(&renderPass, frameBuffer, extent, clearValues, VK_SUBPASS_CONTENTS_INLINE);

	commandBuffer->cmdBindPipeline(&pipeline);
	commandBuffer->cmdDraw(36, 1, 0, 0);

	commandBuffer->cmdEndRenderPass();

	commandPool->endSingleTimeCommand(commandBuffer);

	sampler.destroy();
	vkDestroyFramebuffer(VulkanInstance::get()->getLogicalDevice(), frameBuffer, nullptr);
	descLayout.destroy();
	renderPass.destroy();
	shader.destroy();
	pipeline.destroy();

	Image::TransistionDesc transitionDesc;
	transitionDesc.format = format;
	transitionDesc.layerCount = 1;
	transitionDesc.pool = commandPool;
	transitionDesc.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	transitionDesc.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	brdfLutTexture->image.transistionLayout(transitionDesc);

	return brdfLutTexture;
}

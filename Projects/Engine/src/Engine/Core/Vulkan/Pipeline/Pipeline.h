#pragma once

#include <vulkan/vulkan.h>
#include "PipelineInfo.h"

namespace ym
{
	class PushConstants;
	class DescriptorLayout;
	class RenderPass;
	class Shader;

	class Pipeline
	{
	public:
		enum class Type {
			GRAPHICS = VK_PIPELINE_BIND_POINT_GRAPHICS,
			COMPUTE = VK_PIPELINE_BIND_POINT_COMPUTE
		};
	public:
		Pipeline();
		~Pipeline();

		// Creates the pipeline with default values or the values that has been set before this call
		void init(Type type, Shader* shader);
		void destroy();

		// Set the layouts which will be used
		void setDescriptorLayouts(const std::vector<DescriptorLayout>& descriptorLayouts);
		// Set push constants which will be used
		void setPushConstants(const PushConstants& pushConstants);
		// Set a single push constant.
		void setPushConstant(VkPushConstantRange range);
		// Enables or disables wireframe at creation time
		void setWireframe(bool enable);
		// Sets the necessary information for the graphics pipeline
		void setGraphicsPipelineInfo(VkExtent2D extent, RenderPass* renderPass);
		// Enable dynamic states. If this is not used, no state will be dynamic!
		void setDynamicState(const std::vector<VkDynamicState>& dynamicStates);
		// Overwrites the selected default pipeline infos, has to be done before init
		void setPipelineInfo(PipelineInfoFlag flags, PipelineInfo info);
		VkPipeline getPipeline() const { return this->pipeline; }
		Type getType() const { return this->type; }
		VkPipelineLayout getPipelineLayout() const { return this->pipelineLayout; }

	private:
		void createGraphicsPipeline();
		void createComputePipeline();

		VkPipeline pipeline;
		VkPipelineLayout pipelineLayout;
		VkPolygonMode polyMode;
		RenderPass* renderPass;
		Shader* shader;
		VkExtent2D extent;
		Type type;
		bool graphicsPipelineInitilized = false;
		std::vector<VkDescriptorSetLayout> layouts;
		std::vector<VkPushConstantRange> pushConstantRanges;
		std::vector<VkDynamicState> dynamicStates;

		// Pipeline create info
		PipelineInfo graphicsPipelineInfo;
		PipelineInfoFlag graphicsPipelineInfoFlags;
	};
}
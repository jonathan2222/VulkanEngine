#pragma once

#include "stdafx.h"

namespace ym
{
	class Shader
	{
	public:
		enum Type { VERTEX, FRAGMENT, COMPUTE };

		Shader();
		virtual ~Shader();
		
		void init();
		void destroy();

		void setName(const std::string& name);
		void addStage(Type type, const std::string& shaderName);

		VkPipelineShaderStageCreateInfo getShaderCreateInfo(Type type) const;
		std::vector<VkPipelineShaderStageCreateInfo> getShaderCreateInfos();
		std::string getName() const;
		uint32_t getId() const;

	private:
		struct Stage
		{
			VkPipelineShaderStageCreateInfo stageCreateInfo;
			VkShaderModule shaderModule;
		};

		VkShaderModule createShaderModule(Type type, const std::vector<char>& bytecode);
		VkPipelineShaderStageCreateInfo createShaderStageCreateInfo(Type type, VkShaderModule shaderModule);

		static VkShaderStageFlagBits typeToStageFlag(Type type);
		static std::string typeToStr(Type type);
		static std::vector<char> readFile(const std::string& filename);

	private:
		std::unordered_map<Type, Stage> shaderStages;
		std::unordered_map<Type, std::string> shaderStageFilePaths;

		std::string name;
		uint32_t id;
	};
}
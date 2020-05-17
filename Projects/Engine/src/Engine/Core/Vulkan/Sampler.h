#pragma once

#include <vulkan/vulkan.h>

namespace ym
{
	class Sampler
	{
	public:
		Sampler();
		~Sampler();

		void init(VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode uWrap, VkSamplerAddressMode vWrap, float mipLevels);
		void destroy();

		VkSampler getSampler() const { return this->sampler; }

	private:
		VkSampler sampler;
	};
}
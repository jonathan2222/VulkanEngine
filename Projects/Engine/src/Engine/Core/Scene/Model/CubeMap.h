#pragma once

#include "stdafx.h"
#include "Engine/Core/Vulkan/Buffers/Memory.h"
#include "Engine/Core/Vulkan/Buffers/Buffer.h"
#include "Engine/Core/Vulkan/Sampler.h"
#include "Engine/Core/Vulkan/Texture.h"

namespace ym
{
	class SwapChain;
	class CommandPool;
	class CubeMap
	{
	public:
		CubeMap();
		~CubeMap();

		void init(float scale, const std::string& texturePath);
		void destroy();

		Texture* getTexture();
		Sampler* getSampler();
		Buffer* getVertexBuffer();

		uint64_t getUniqueId() const;

		VkDescriptorSet descriptorSet{ VK_NULL_HANDLE }; // This is set by CubeMapRenderer.
	private:
		Buffer cubeBuffer;
		Memory cubeMemory;

		Texture* cubemapTexture;
		Sampler cubemapSampler;

		uint64_t uniqueId;

	private:
		// Cube verticies
		glm::vec4 cube[36] = {
			{-1.0f,-1.0f,-1.0f, 1.0f},
			{-1.0f,-1.0f, 1.0f,	1.0f},
			{-1.0f, 1.0f, 1.0f, 1.0f},
			{ 1.0f, 1.0f,-1.0f, 1.0f},
			{-1.0f,-1.0f,-1.0f,	1.0f},
			{-1.0f, 1.0f,-1.0f, 1.0f},
			{ 1.0f,-1.0f, 1.0f,	1.0f},
			{-1.0f,-1.0f,-1.0f,	1.0f},
			{ 1.0f,-1.0f,-1.0f,	1.0f},
			{ 1.0f, 1.0f,-1.0f,	1.0f},
			{ 1.0f,-1.0f,-1.0f,	1.0f},
			{-1.0f,-1.0f,-1.0f,	1.0f},
			{-1.0f,-1.0f,-1.0f,	1.0f},
			{-1.0f, 1.0f, 1.0f,	1.0f},
			{-1.0f, 1.0f,-1.0f,	1.0f},
			{ 1.0f,-1.0f, 1.0f,	1.0f},
			{-1.0f,-1.0f, 1.0f,	1.0f},
			{-1.0f,-1.0f,-1.0f,	1.0f},
			{-1.0f, 1.0f, 1.0f,	1.0f},
			{-1.0f,-1.0f, 1.0f,	1.0f},
			{ 1.0f,-1.0f, 1.0f,	1.0f},
			{ 1.0f, 1.0f, 1.0f,	1.0f},
			{ 1.0f,-1.0f,-1.0f,	1.0f},
			{ 1.0f, 1.0f,-1.0f,	1.0f},
			{ 1.0f,-1.0f,-1.0f, 1.0f},
			{ 1.0f, 1.0f, 1.0f, 1.0f},
			{ 1.0f,-1.0f, 1.0f, 1.0f},
			{ 1.0f, 1.0f, 1.0f, 1.0f},
			{ 1.0f, 1.0f,-1.0f, 1.0f},
			{-1.0f, 1.0f,-1.0f,	1.0f},
			{ 1.0f, 1.0f, 1.0f, 1.0f},
			{-1.0f, 1.0f,-1.0f,	1.0f},
			{-1.0f, 1.0f, 1.0f,	1.0f},
			{ 1.0f, 1.0f, 1.0f, 1.0f},
			{-1.0f, 1.0f, 1.0f,	1.0f},
			{ 1.0f,-1.0f, 1.0f, 1.0f}
		};
	};
}
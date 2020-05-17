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

		/*
			Will load 6 textures and create a cubemap from them.
		*/
		void init(float scale, const std::string& texturePath);

		/*
			Creates the vertices and store a texture but does NOT create a sampler.
		*/
		void init(float scale, Texture* texture);

		/*
			Only creates the vertices.
		*/
		void init(float scale);

		void destroy();

		/*
			Set the texture.
		*/
		void setTexture(Texture* texturePtr);

		/*
			Set the texture and creates a sampler.
		*/
		void setTextureCreateSampler(Texture* texturePtr);

		/*
			Set the texture and creates a sampler with specified mipLevels.
		*/
		void setTexture(Texture* texturePtr, uint32_t mipLevels);

		Texture* getTexture();
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
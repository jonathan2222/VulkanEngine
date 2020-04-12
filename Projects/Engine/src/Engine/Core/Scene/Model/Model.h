#pragma once

#include "../../Vulkan/Buffers/Memory.h"
#include "../../Vulkan/Buffers/Buffer.h"
#include "../../Vulkan/Buffers/UniformBuffer.h"
#include "../../Vulkan/Texture.h"
#include "../../Vulkan/Sampler.h"
#include "../Vertex.h"
#include "Material.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace ym
{
	struct Primitive
	{
		uint32_t firstIndex{ 0 };
		uint32_t indexCount{ 0 };
		uint32_t vertexCount{ 0 };
		bool hasIndices{ false };
		Material* material{ nullptr };
	};

	class Mesh
	{
	public:
		std::string name{ "Mesh" };
		std::vector<Primitive> primitives;
	};

	class Model
	{
	public:
		struct Node
		{
			struct NodeData
			{
				glm::mat4 transform;
			};

			bool hasMesh{ false };
			Mesh mesh;
			glm::vec3 translation;
			glm::quat rotation;
			glm::vec3 scale;
			glm::mat4 matrix; // Combination of rotation, translation and scale.
			std::vector<Node> children;
			Node* parent{ nullptr };
			Model* model{ nullptr };
			std::vector<VkDescriptorSet> descriptorSets; // Only holds matrix right now.
			std::vector<UniformBuffer> uniformBuffers;	 // For the matrix.
		};

	public:
		Model();
		~Model();

		void destroy();

		uint32_t uniqueId;
		bool hasLoaded{ false };

		// Layout
		std::vector<Node> nodes;
		uint32_t numMeshes;

		// Data
		std::vector<uint32_t> indices;
		std::vector<Vertex> vertices;
		Buffer indexBuffer;
		Buffer vertexBuffer;
		Memory bufferMemory;

		bool hasImageMemory{ false };
		std::vector<std::vector<uint8_t>> imageData;
		std::vector<Texture> textures;
		Memory imageMemory;

		std::vector<Sampler> samplers;

		bool hasMaterialMemory{ false };
		Memory materialMemory;
		std::vector<Material> materials;

	private:
		void destroyNode(Node& node);
	};
}
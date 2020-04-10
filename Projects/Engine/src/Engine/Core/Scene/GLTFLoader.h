#pragma once

// ------------ TEMP ------------
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "glTF/tiny_gltf.h"

#include "Engine/Core/Vulkan/Buffers/Buffer.h"
#include "Engine/Core/Vulkan/Buffers/Memory.h"
#include "Engine/Core/Vulkan/Texture.h"
#include "Model/Model.h"

#include <queue>
#include <mutex>
#include <set>

namespace ym
{
	class CommandPool;
	class CommandBuffer;
	class Pipeline;

	class GLTFLoader
	{
	public:
		struct StagingBuffers
		{
			Buffer geometryBuffer;
			Buffer imageBuffer;
			Memory geometryMemory;
			Memory imageMemory;

			void initMemory()
			{
				this->geometryMemory.init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
				this->imageMemory.init(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			}

			void destroy()
			{
				this->geometryBuffer.destroy();
				this->imageBuffer.destroy();
				this->geometryMemory.destroy();
				this->imageMemory.destroy();
			}
		};

	public:
		/*
			Initialize default data.
		*/
		static void init();

		/*
			Destroy default data.
		*/
		static void destroy();

		/*
			[This is only needed if loadOnThread is used]
			Transfer data to GPU if it has finished loading on the thread.
		*/
		static void update();

		/*
			[Only works if update is called each frame]
			Load a model from a thread to the GPU. This will perform staging.
		*/
		static void loadOnThread(const std::string& filePath, Model* model);

		/*
			Load a model to the GPU. This will perform staging.
		*/
		static void load(const std::string& filePath, Model* model);

		/*
			Create buffers and load the model data to RAM.
		*/
		static void loadToRAM(const std::string& filePath, Model* model, StagingBuffers* stagingBuffers);

		/*
			Transfer model data from RAM to the GPU.
		*/
		static void transferToGPU(CommandPool* transferCommandPool, Model* model, StagingBuffers* stagingBuffers);

	private:
		/*
			Creates default data such as a default texture and sampler, when it is missing.
		*/
		static void initDefaultData(CommandPool* transferCommandPool);

		/*
			Destroy the default data.
		*/
		static void destroyDefaultData();

		static void loadTextures(std::string& folderPath, Model& model, tinygltf::Model& gltfModel, StagingBuffers* stagingBuffers);
		static void loadImageData(std::string& folderPath, tinygltf::Image& image, tinygltf::Model& gltfModel, std::vector<uint8_t>& data);
		static void loadSamplerData(tinygltf::Sampler& samplerGltf, Sampler& sampler);
		static void loadMaterials(Model& model, tinygltf::Model& gltfModel);
		static void loadNode(Model& model, Model::Node* node, tinygltf::Model& gltfModel, tinygltf::Node& gltfNode, std::string indents);

		static void loadModel(Model& model, const std::string& filePath, StagingBuffers* stagingBuffers);
		static void loadScenes(Model& model, tinygltf::Model& gltfModel, StagingBuffers* stagingBuffers);

	private:
		static Model model;
		static tinygltf::TinyGLTF loader;

		struct DefaultData
		{
			Texture* texture;
			Sampler sampler;
		};
		static DefaultData defaultData;
		static CommandPool* commandPool;

		struct ThreadData
		{
			Model* model{ nullptr };
			StagingBuffers* stagingBuffers{ nullptr };
		};
		static std::queue<ThreadData> modelQueue;
		static std::set<Model*> modelSet;
		static std::mutex mutex;
	};
}
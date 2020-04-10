#include "stdafx.h"

// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_JSON

#include "GLTFLoader.h"
#include "Engine/Core/Vulkan/VulkanInstance.h"
#include "Engine/Core/Vulkan/SwapChain.h"
#include "Engine/Core/Vulkan/CommandPool.h"
#include "Engine/Core/Vulkan/CommandBuffer.h"

#include "Engine/Core/Vulkan/Pipeline/Pipeline.h"
#include "Engine/Core/Vulkan/Pipeline/DescriptorManager.h"

#include <glm/gtc/matrix_transform.hpp> // translate() and scale()
#include <glm/gtx/quaternion.hpp>		// toMat4()
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>			// make_vec3(), make_mat4()

#include "Engine/Core/Vulkan/Factory.h"
#include "Engine/Core/Application/LayerManager.h"

#include "Engine/Core/Threading/ThreadManager.h"
#include "Engine/Core/Threading/ThreadIndices.h"

#include "Engine/Core/Vulkan/CommandPool.h"

namespace ym
{
	Model GLTFLoader::model = Model();
	tinygltf::TinyGLTF GLTFLoader::loader = tinygltf::TinyGLTF();
	GLTFLoader::DefaultData GLTFLoader::defaultData;
	CommandPool* GLTFLoader::commandPool = nullptr;

	std::queue<GLTFLoader::ThreadData> GLTFLoader::modelQueue;
	std::mutex GLTFLoader::mutex;
	std::set<Model*> GLTFLoader::modelSet; // Used for debugging to check if trying to load to the same model from a thread.

	void GLTFLoader::init()
	{
		commandPool = new CommandPool();
		commandPool->init(CommandPool::Queue::GRAPHICS, 0);
		initDefaultData(commandPool);
	}

	void GLTFLoader::destroy()
	{
		destroyDefaultData();
		commandPool->destroy();
		SAFE_DELETE(commandPool);
	}

	void GLTFLoader::update()
	{
		CommandPool* graphicsPool = &LayerManager::get()->getCommandPools()->graphicsPool;
		std::lock_guard<std::mutex> lck(mutex);
		while (modelQueue.empty() == false)
		{
			ThreadData& data = modelQueue.front();
			data.stagingBuffers->initMemory();
			transferToGPU(graphicsPool, data.model, data.stagingBuffers);
			data.stagingBuffers->destroy();
			SAFE_DELETE(data.stagingBuffers);
			modelQueue.pop();
			modelSet.erase(data.model);
		}
	}

	void GLTFLoader::loadOnThread(const std::string& filePath, Model* model)
	{
		if (modelSet.insert(model).second)
		{
			ThreadManager::addWork((uint32_t)ThreadIndices::GLTF_LOADER, [=]() {
				ThreadData threadData;
				threadData.model = model;
				threadData.stagingBuffers = new StagingBuffers();
				loadToRAM(filePath, model, threadData.stagingBuffers);

				{
					std::lock_guard<std::mutex> lck(mutex);
					modelQueue.push(threadData);
				}
			});
		}
		else
		{
			YM_LOG_ERROR("Cannot load a model to an existing model pointer!");
		}
	}

	void GLTFLoader::load(const std::string& filePath, Model* model)
	{
		if (model->vertices.empty())
		{
			CommandPool* graphicsPool = &LayerManager::get()->getCommandPools()->graphicsPool;
			StagingBuffers stagingBuffers;
			loadToRAM(filePath, model, &stagingBuffers);
			stagingBuffers.initMemory();
			transferToGPU(graphicsPool, model, &stagingBuffers);
			stagingBuffers.destroy();
		}
		else
		{
			YM_LOG_ERROR("Cannot load a model to an existing model pointer!");
		}
	}

	void GLTFLoader::loadToRAM(const std::string & filePath, Model * model, StagingBuffers * stagingBuffers)
	{
		loadModel(*model, filePath, stagingBuffers);
	}

	void GLTFLoader::transferToGPU(CommandPool * transferCommandPool, Model * model, StagingBuffers * stagingBuffers)
	{
		// Transfer all images their coresponding textures
		uint64_t totalTextureSize = 0;
		for (uint32_t textureIndex = 0; textureIndex < model->textures.size(); textureIndex++)
		{
			Texture& texture = model->textures[textureIndex];
			const uint32_t wh = texture.textureDesc.width * texture.textureDesc.height;
			const uint64_t size = wh * 4;
			totalTextureSize += size;

			// Transfer the data to the buffer.
			std::vector<uint8_t> data = model->imageData[textureIndex];
			uint8_t* img = data.data();
			stagingBuffers->imageMemory.directTransfer(&stagingBuffers->imageBuffer, (void*)img, size, textureIndex * size);

			// Setup a buffer copy region for the transfer.
			std::vector<VkBufferImageCopy> bufferCopyRegions;
			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = 0;
			bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = texture.textureDesc.width;
			bufferCopyRegion.imageExtent.height = texture.textureDesc.height;
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = textureIndex * size;
			bufferCopyRegions.push_back(bufferCopyRegion);

			// Transfer data to texture memory.
			Image::TransistionDesc desc;
			desc.format = texture.textureDesc.format;
			desc.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			desc.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			desc.pool = transferCommandPool;
			desc.layerCount = 1;
			
			texture.image.transistionLayout(desc);
			texture.image.copyBufferToImage(&stagingBuffers->imageBuffer, transferCommandPool, bufferCopyRegions);
			desc.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			desc.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			texture.image.transistionLayout(desc);
		}
		model->imageData.clear();

		// Transfer data to buffers
		uint32_t indicesSize = (uint32_t)(model->indices.size() * sizeof(uint32_t));
		uint32_t verticesSize = (uint32_t)(model->vertices.size() * sizeof(Vertex));
		if (indicesSize > 0)
			stagingBuffers->geometryMemory.directTransfer(&stagingBuffers->geometryBuffer, (const void*)model->indices.data(), indicesSize, (Offset)0);
		stagingBuffers->geometryMemory.directTransfer(&stagingBuffers->imageBuffer, (const void*)model->vertices.data(), verticesSize, (Offset)indicesSize);

		// Create memory and buffers.
		std::vector<uint32_t> queueIndices = { findQueueIndex(VK_QUEUE_TRANSFER_BIT, VulkanInstance::get()->getPhysicalDevice()) };
		if (indicesSize > 0)
		{
			model->indexBuffer.init(indicesSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queueIndices);
			model->bufferMemory.bindBuffer(&model->indexBuffer);
		}

		model->vertexBuffer.init(verticesSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queueIndices);
		model->bufferMemory.bindBuffer(&model->vertexBuffer);

		// Create memory with the binded buffers
		model->bufferMemory.init(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		// Copy data to the new buffers.
		CommandBuffer* cbuff = transferCommandPool->beginSingleTimeCommand();

		// Copy indices data.
		if (indicesSize > 0)
		{
			VkBufferCopy region = {};
			region.srcOffset = 0;
			region.dstOffset = 0;
			region.size = indicesSize;
			cbuff->cmdCopyBuffer(stagingBuffers->geometryBuffer.getBuffer(), model->indexBuffer.getBuffer(), 1, &region);
		}

		// Copy vertex data.
		VkBufferCopy region = {};
		region.srcOffset = indicesSize;
		region.dstOffset = 0;
		region.size = verticesSize;
		cbuff->cmdCopyBuffer(stagingBuffers->geometryBuffer.getBuffer(), model->vertexBuffer.getBuffer(), 1, &region);

		transferCommandPool->endSingleTimeCommand(cbuff);

		model->hasLoaded = true;
	}

	void GLTFLoader::initDefaultData(CommandPool* transferCommandPool)
	{
		// Default 1x1 white texture
		uint8_t pixel[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
		TextureDesc textureDesc;
		textureDesc.width = 1;
		textureDesc.height = 1;
		textureDesc.format = VK_FORMAT_R8G8B8A8_UNORM;
		textureDesc.data = &pixel[0];
		defaultData.texture = ym::Factory::createTexture(textureDesc, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_QUEUE_GRAPHICS_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
		ym::Factory::transferData(defaultData.texture, transferCommandPool);

		// Sampler
		Sampler& sampler = defaultData.sampler;
		sampler.init(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT);

		// Set descriptor
		ym::Factory::applyTextureDescriptor(defaultData.texture, &sampler);
	}

	void GLTFLoader::destroyDefaultData()
	{
		defaultData.texture->destroy();
		SAFE_DELETE(defaultData.texture);
		defaultData.sampler.destroy();
	}

	void GLTFLoader::loadTextures(std::string & folderPath, Model & model, tinygltf::Model & gltfModel, StagingBuffers * stagingBuffers)
	{
		YM_LOG_INFO("Textures:");
		if (gltfModel.textures.empty()) YM_LOG_INFO(" ->No textures");
		else model.hasImageMemory = true;

		model.imageData.resize(gltfModel.textures.size());
		model.textures.resize(gltfModel.textures.size());
		VkFormat format = VK_FORMAT_R8G8B8A8_UNORM; // Assume all have the same layout. (loadImageData will force them to have 4 components, each being one byte)
		for (size_t textureIndex = 0; textureIndex < gltfModel.textures.size(); textureIndex++)
		{
			tinygltf::Texture& textureGltf = gltfModel.textures[textureIndex];
			tinygltf::Image& image = gltfModel.images[textureGltf.source];
			YM_LOG_INFO(" ->[{0}] name: {1} uri: {2} bits: {3} comp: {4} w: {5} h: {6}", textureIndex, textureGltf.name.c_str(), image.uri.c_str(), image.bits, image.component, image.width, image.height);

			loadImageData(folderPath, image, gltfModel, model.imageData[textureIndex]);

			Texture& texture = model.textures[textureIndex];
			texture.image.init(image.width, image.height, format, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, { VulkanInstance::get()->getGraphicsQueue().queueIndex }, 0, 1);
			model.imageMemory.bindTexture(&texture);
			texture.textureDesc.width = (uint32_t)image.width;
			texture.textureDesc.height = (uint32_t)image.height;
			texture.textureDesc.format = format;
		}
		model.imageMemory.init(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		for (Texture& texture : model.textures)
		{
			texture.imageView.init(texture.image.getImage(), VK_IMAGE_VIEW_TYPE_2D, format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
			texture.descriptor.imageView = texture.imageView.getImageView();
			texture.descriptor.imageLayout = texture.image.getLayout();
		}

		uint64_t textureSize = 0;
		for (Texture& texture : model.textures)
			textureSize += texture.textureDesc.width * texture.textureDesc.height * 4;
		stagingBuffers->imageBuffer.init(textureSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, { VulkanInstance::get()->getGraphicsQueue().queueIndex });
		stagingBuffers->imageMemory.bindBuffer(&stagingBuffers->imageBuffer);

		// TODO: Load all samplers into memory and index them when needed.
		YM_LOG_INFO("Samplers:");
		if (gltfModel.samplers.empty()) YM_LOG_INFO(" ->No samplers");
		model.samplers.resize(gltfModel.samplers.size());
		for (size_t samplerIndex = 0; samplerIndex < gltfModel.samplers.size(); samplerIndex++)
		{
			tinygltf::Sampler& samplerGltf = gltfModel.samplers[samplerIndex];
			YM_LOG_INFO(" ->[{0}] name: {1} magFilter: {2}, minFilter: {3}", samplerIndex, samplerGltf.name.c_str(), samplerGltf.magFilter, samplerGltf.minFilter);

			Sampler& sampler = model.samplers[samplerIndex];
			loadSamplerData(samplerGltf, sampler);
		}
	}

	void GLTFLoader::loadImageData(std::string & folderPath, tinygltf::Image & image, tinygltf::Model & gltfModel, std::vector<uint8_t> & data)
	{
		const int numComponents = 4;
		const int size = image.width * image.height * numComponents;
		if (!image.uri.empty())
		{
			// Fetch data from a file.
			int width, height;
			int channels;
			std::string path = folderPath + image.uri;
			uint8_t* imgData = static_cast<uint8_t*>(stbi_load(path.c_str(), &width, &height, &channels, numComponents));
			if (imgData == nullptr)
				YM_LOG_ERROR("Failed to load texture! could not find file, path: {}", path.c_str());
			else {
				if (width == image.width && height == image.height && image.bits == 8)
				{
					YM_LOG_INFO("Loaded texture: {} successfully!", path.c_str());
					data.insert(data.end(), imgData, imgData + size);
				}
				else YM_LOG_ERROR("Failed to load texture! Dimension or bit-depth do not match the specified values! Path: {}", path.c_str());
				delete[] imgData;
			}
		}
		else
		{
			// Fetch data from a buffer.
			tinygltf::BufferView& view = gltfModel.bufferViews[image.bufferView];
			tinygltf::Buffer& buffer = gltfModel.buffers[view.buffer];
			uint8_t* dataPtr = &buffer.data.at(0) + view.byteOffset;

			int width, height;
			int channels;
			uint8_t* imgData = static_cast<uint8_t*>(stbi_load_from_memory(dataPtr, (int)view.byteLength, &width, &height, &channels, numComponents));
			if (imgData == nullptr)
				YM_LOG_ERROR("Failed to load embedded texture! could not find file!");
			else {
				if (width == image.width && height == image.height && image.bits == 8)
				{
					YM_LOG_INFO("Loaded embedded texture successfully!");
					data.insert(data.end(), imgData, imgData + size);
				}
				else YM_LOG_ERROR("Failed to load embedded texture! Dimension or bit-depth do not match the specified values!");
				delete[] imgData;
			}
		}
	}

	void GLTFLoader::loadSamplerData(tinygltf::Sampler & samplerGltf, Sampler & sampler)
	{
		VkFilter minFilter = VK_FILTER_NEAREST;
		switch (samplerGltf.minFilter)
		{
		case 0: // NEAREST
		case 2: // NEAREST
		case 4: // NEAREST
			minFilter = VK_FILTER_NEAREST;
			break;
		case 1: // LINEAR
		case 3: // LINEAR
		case 5: // LINEAR
			minFilter = VK_FILTER_LINEAR;
			break;
		}
		VkFilter magFilter = VK_FILTER_NEAREST;
		switch (samplerGltf.magFilter)
		{
		case 0: // NEAREST
			minFilter = VK_FILTER_NEAREST;
			break;
		case 1: // LINEAR
			minFilter = VK_FILTER_LINEAR;
			break;
		}
		VkSamplerAddressMode uWrap = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		switch (samplerGltf.wrapS)
		{
		case 0: // CLAMP_TO_EDGE
			uWrap = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			break;
		case 1: // MIRRORED_REPEAT
			uWrap = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			break;
		case 2: // REPEAT
			uWrap = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			break;
		}
		VkSamplerAddressMode vWrap = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		switch (samplerGltf.wrapT)
		{
		case 0: // CLAMP_TO_EDGE
			vWrap = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			break;
		case 1: // MIRRORED_REPEAT
			vWrap = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			break;
		case 2: // REPEAT
			vWrap = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			break;
		}
		sampler.init(minFilter, magFilter, uWrap, vWrap);
	}

	void GLTFLoader::loadMaterials(Model & model, tinygltf::Model & gltfModel)
	{
		// TODO: Load all materials into memory and index them when needed.
		YM_LOG_INFO("Materials:");
		if (gltfModel.materials.empty()) YM_LOG_INFO(" ->No materials");
		else model.hasMaterialMemory = true;

		model.materials.resize(gltfModel.materials.size());
		for (size_t materialIndex = 0; materialIndex < gltfModel.materials.size(); materialIndex++)
		{
			tinygltf::Material& materialGltf = gltfModel.materials[materialIndex];
			YM_LOG_INFO(" ->[{0}] name: {1}", materialIndex, materialGltf.name.c_str());
			tinygltf::PbrMetallicRoughness& pbrMR = materialGltf.pbrMetallicRoughness;
			YM_LOG_INFO("    ->[PbrMetallicRoughness]");
			YM_LOG_INFO("        ->metallicFactor: {}, roughnessFactor: {}", pbrMR.metallicFactor, pbrMR.roughnessFactor);
			tinygltf::TextureInfo& baseColorTexture = pbrMR.baseColorTexture;
			YM_LOG_INFO("        ->[BaseColorTexture] index: {}, texCoord: {}", baseColorTexture.index, baseColorTexture.texCoord);
			tinygltf::TextureInfo& metallicRoughnessTexture = pbrMR.metallicRoughnessTexture;
			YM_LOG_INFO("        ->[MetallicRoughnessTexture] index: {}, texCoord: {}", metallicRoughnessTexture.index, metallicRoughnessTexture.texCoord);
			YM_LOG_INFO("        ->[BaseColorFactor] value: ({}, {}, {}, {})", pbrMR.baseColorFactor[0], pbrMR.baseColorFactor[1], pbrMR.baseColorFactor[2], pbrMR.baseColorFactor[3]);

			tinygltf::NormalTextureInfo& normalTexture = materialGltf.normalTexture;
			YM_LOG_INFO("    ->[NormalTextureInfo] index: {}, texCoord: {}, scale: {}", normalTexture.index, normalTexture.texCoord, normalTexture.scale);
			tinygltf::OcclusionTextureInfo occlusionTexture = materialGltf.occlusionTexture;
			YM_LOG_INFO("    ->[OcclusionTextureInfo] index: {}, texCoord: {}, strength: {}", occlusionTexture.index, occlusionTexture.texCoord, occlusionTexture.strength);
			tinygltf::TextureInfo emissiveTexture = materialGltf.emissiveTexture;
			YM_LOG_INFO("    ->[EmissiveTexture] index: {}, texCoord: {}", emissiveTexture.index, emissiveTexture.texCoord);
			YM_LOG_INFO("    ->[EmissiveFactor] value: ({}, {}, {})", materialGltf.emissiveFactor[0], materialGltf.emissiveFactor[1], materialGltf.emissiveFactor[2]);

			// Construct material structure.
			Material& material = model.materials[materialIndex];
			material.index = (uint32_t)materialIndex;
			/*material.pbrMR.metallicFactor = (float)pbrMR.metallicFactor;
			material.pbrMR.roughnessFactor = (float)pbrMR.roughnessFactor;
			material.pbrMR.baseColorTexture.index = baseColorTexture.index;
			material.pbrMR.baseColorTexture.texCoord = baseColorTexture.texCoord;
			material.pbrMR.metallicRoughnessTexture.index = metallicRoughnessTexture.index;
			material.pbrMR.metallicRoughnessTexture.texCoord = metallicRoughnessTexture.texCoord;
			material.pbrMR.baseColorFactor = glm::make_vec4(pbrMR.baseColorFactor.data());
			material.normalTexture.index = normalTexture.index;
			material.normalTexture.texCoord = normalTexture.texCoord;
			material.normalTexture.scale = normalTexture.scale;
			material.occlusionTexture.index = occlusionTexture.index;
			material.occlusionTexture.texCoord = occlusionTexture.texCoord;
			material.occlusionTexture.strength = occlusionTexture.strength;
			material.emissiveTexture.index = emissiveTexture.index;
			material.emissiveTexture.texCoord = emissiveTexture.texCoord;
			material.emissiveFactor = glm::make_vec3(materialGltf.emissiveFactor.data());
			*/

			auto getTex = [&](int index)->Material::Tex {
				Texture* texture = index != -1 ? &model.textures[index] : defaultData.texture;
				Sampler* sampler = (index != -1 && gltfModel.textures[index].sampler != -1) ? &model.samplers[gltfModel.textures[index].sampler] : &defaultData.sampler;
				return { texture, sampler };
			};

			auto getTeCoord = [&](int index, int texCoord)->int {
				if (texCoord == 1 && index != -1) YM_LOG_WARN("Texture coordinate 1 is not supported!");
				return index != -1 ? texCoord : -1;
			};

			material.baseColorTexture = getTex(baseColorTexture.index);
			material.emissiveTexture = getTex(emissiveTexture.index);
			material.metallicRoughnessTexture = getTex(metallicRoughnessTexture.index);
			material.normalTexture = getTex(normalTexture.index);
			material.occlusionTexture = getTex(occlusionTexture.index);

			// Copy data to push structure.
			material.pushData.baseColorFactor = glm::make_vec4(pbrMR.baseColorFactor.data());
			material.pushData.baseColorTextureCoord = getTeCoord(baseColorTexture.index, baseColorTexture.texCoord);
			material.pushData.emissiveFactor = glm::vec4(glm::make_vec3(materialGltf.emissiveFactor.data()), 0.0f);
			material.pushData.emissiveTextureCoord = getTeCoord(emissiveTexture.index, emissiveTexture.texCoord);
			material.pushData.metallicFactor = (float)pbrMR.metallicFactor;
			material.pushData.roughnessFactor = (float)pbrMR.roughnessFactor;
			material.pushData.metallicRoughnessTextureCoord = getTeCoord(metallicRoughnessTexture.index, metallicRoughnessTexture.texCoord);
			material.pushData.normalTextureCoord = getTeCoord(normalTexture.index, normalTexture.texCoord);
			material.pushData.occlusionTextureCoord = getTeCoord(occlusionTexture.index, occlusionTexture.texCoord);
		}
	}

	void GLTFLoader::loadNode(Model & model, Model::Node * node, tinygltf::Model & gltfModel, tinygltf::Node & gltfNode, std::string indents)
	{
		//JAS_INFO("{0}->Node [{1}]", indents.c_str(), gltfNode.name.c_str());

		// Set scale
		node->model = &model;
		if (!gltfNode.scale.empty())
			node->scale = glm::vec3((float)gltfNode.scale[0], (float)gltfNode.scale[1], (float)gltfNode.scale[2]);
		else node->scale = glm::vec3(1.0f);

		// Set translation
		if (!gltfNode.translation.empty())
			node->translation = glm::vec3((float)gltfNode.translation[0], (float)gltfNode.translation[1], (float)gltfNode.translation[2]);
		else node->translation = glm::vec3(0.0f);

		// Set rotation
		if (!gltfNode.rotation.empty())
			node->rotation = glm::quat((float)gltfNode.rotation[3], (float)gltfNode.rotation[0], (float)gltfNode.rotation[1], (float)gltfNode.rotation[2]);
		else node->rotation = glm::quat();

		// Compute the matrix
		if (!gltfNode.matrix.empty())
			node->matrix = glm::make_mat4(gltfNode.matrix.data());
		else
		{
			glm::mat4 t = glm::translate(node->translation);
			glm::mat4 s = glm::scale(node->scale);
			glm::mat4 r = glm::mat4(node->rotation);
			node->matrix = t * r * s;
		}

		//if (!gltfNode.children.empty())
		//	JAS_INFO("{0} ->Children:", indents.c_str());
		node->children.resize(gltfNode.children.size());
		for (size_t childIndex = 0; childIndex < gltfNode.children.size(); childIndex++)
		{
			// Set parent
			node->children[childIndex].parent = node;
			// Load child node
			tinygltf::Node& child = gltfModel.nodes[gltfNode.children[childIndex]];
			loadNode(model, &node->children[childIndex], gltfModel, child, indents + "  ");
		}

		// Check if it has a mesh
		node->hasMesh = gltfNode.mesh != -1;
		if (node->hasMesh)
		{
			model.numMeshes++;

			tinygltf::Mesh gltfMesh = gltfModel.meshes[gltfNode.mesh];
			Mesh& mesh = node->mesh;
			mesh.name = gltfMesh.name;
			//JAS_INFO("{0} ->Has mesh: {1}", indents.c_str(), mesh.name.c_str());

			mesh.primitives.resize(gltfMesh.primitives.size());
			for (size_t i = 0; i < gltfMesh.primitives.size(); i++)
			{
				Primitive& primitive = mesh.primitives[i];
				primitive.firstIndex = static_cast<uint32_t>(model.indices.size());
				uint32_t vertexStart = static_cast<uint32_t>(model.vertices.size()); // Used for indices.

				tinygltf::Primitive& gltfPrimitive = gltfMesh.primitives[i];

				// Material
				primitive.material = &model.materials[gltfPrimitive.material];

				// Indicies
				primitive.hasIndices = gltfPrimitive.indices != -1;
				if (primitive.hasIndices)
				{
					//JAS_INFO("{0}  ->Has indices", indents.c_str());
					tinygltf::Accessor& indAccessor = gltfModel.accessors[gltfPrimitive.indices];
					tinygltf::BufferView& indBufferView = gltfModel.bufferViews[indAccessor.bufferView];
					tinygltf::Buffer& indBuffer = gltfModel.buffers[indBufferView.buffer];

					primitive.indexCount = static_cast<uint32_t>(indAccessor.count);
					const void* dataPtr = &indBuffer.data.at(0) + indBufferView.byteOffset;
					for (size_t c = 0; c < indAccessor.count; c++)
					{
						switch (indAccessor.componentType)
						{
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
						{
							const uint8_t* data = static_cast<const uint8_t*>(dataPtr);
							model.indices.push_back(data[c] + vertexStart);
						}
						break;
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
						{
							const uint16_t* data = static_cast<const uint16_t*>(dataPtr);
							model.indices.push_back(data[c] + vertexStart);
						}
						break;
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
						{
							const uint32_t* data = static_cast<const uint32_t*>(dataPtr);
							model.indices.push_back(data[c] + vertexStart);
						}
						break;
						}
					}
				}

				// Begin to fetch attributes and construct the vertices.
				unsigned char* posData = nullptr; // Assume to be vec3
				unsigned char* norData = nullptr; // Assume to be vec3
				unsigned char* uv0Data = nullptr; // Assume to be vec2
				size_t posByteStride = 0;
				size_t norByteStride = 0;
				size_t uv0ByteStride = 0;

				auto& attributes = gltfPrimitive.attributes;

				// POSITION
				//JAS_ASSERT(attributes.find("POSITION") != attributes.end(), "Primitive need to have one POSITION attribute!");
				//JAS_INFO("{0}  ->Has POSITION", indents.c_str());
				tinygltf::Accessor& posAccessor = gltfModel.accessors[attributes["POSITION"]];
				tinygltf::BufferView& posBufferView = gltfModel.bufferViews[posAccessor.bufferView];
				tinygltf::Buffer& posBuffer = gltfModel.buffers[posBufferView.buffer];
				posData = &posBuffer.data.at(0) + posBufferView.byteOffset;
				primitive.vertexCount = static_cast<uint32_t>(posAccessor.count);
				posByteStride = posAccessor.ByteStride(posBufferView);

				// NORMAL
				if (attributes.find("NORMAL") != attributes.end())
				{
					//JAS_INFO("{0}  ->Has NORMAL", indents.c_str());
					tinygltf::Accessor& accessor = gltfModel.accessors[attributes["NORMAL"]];
					tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
					tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
					norData = &buffer.data.at(0) + bufferView.byteOffset;
					norByteStride = accessor.ByteStride(bufferView);
				}

				// TEXCOORD_0
				if (attributes.find("TEXCOORD_0") != attributes.end())
				{
					//JAS_INFO("{0}  ->Has TEXCOORD_0", indents.c_str());
					tinygltf::Accessor& accessor = gltfModel.accessors[attributes["TEXCOORD_0"]];
					tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
					tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
					uv0Data = &buffer.data.at(0) + bufferView.byteOffset;
					uv0ByteStride = accessor.ByteStride(bufferView);
				}

				// Construct vertex data
				for (size_t v = 0; v < posAccessor.count; v++)
				{
					Vertex vert;
					vert.pos = glm::make_vec3((const float*)(posData + v * posByteStride));
					vert.nor = norData ? glm::normalize(glm::make_vec3((const float*)(norData + v * norByteStride))) : glm::vec3(0.0f);
					vert.uv0 = uv0Data ? glm::make_vec2((const float*)(uv0Data + v * uv0ByteStride)) : glm::vec2(0.0f);
					model.vertices.push_back(vert);
				}
			}
		}
	}

	void GLTFLoader::loadModel(Model & model, const std::string & filePath, StagingBuffers * stagingBuffers)
	{
		tinygltf::Model gltfModel;
		bool ret = false;
		size_t pos = filePath.rfind('.');
		if (pos != std::string::npos)
		{
			std::string err;
			std::string warn;

			std::string prefix = filePath.substr(pos);
			if (prefix == ".gltf")
				ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, filePath);
			else if (prefix == ".glb")
				ret = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, filePath); // for binary glTF(.glb)

			if (!warn.empty()) YM_LOG_WARN("GLTF Waring: {0}", warn.c_str());
			if (!err.empty()) YM_LOG_ERROR("GLTF Error: {0}", err.c_str());
		}

		YM_ASSERT(ret, "Failed to parse glTF\n");

		pos = filePath.find_last_of("/\\");
		std::string folderPath = filePath.substr(0, pos + 1);

		loadTextures(folderPath, model, gltfModel, stagingBuffers);
		loadMaterials(model, gltfModel);
		loadScenes(model, gltfModel, stagingBuffers);
	}

	void GLTFLoader::loadScenes(Model & model, tinygltf::Model & gltfModel, StagingBuffers * stagingBuffers)
	{
		model.numMeshes = 0;

		for (auto& scene : gltfModel.scenes)
		{
			//JAS_INFO("Scene: {0}", scene.name.c_str());
			// Load each node in the scene
			model.nodes.resize(scene.nodes.size());
			for (size_t nodeIndex = 0; nodeIndex < scene.nodes.size(); nodeIndex++)
			{
				tinygltf::Node& node = gltfModel.nodes[scene.nodes[nodeIndex]];
				loadNode(model, &model.nodes[nodeIndex], gltfModel, node, " ");
			}
		}

		// Create vertex, index and texture buffer for staging. 
		std::vector<uint32_t> queueIndices = { findQueueIndex(VK_QUEUE_TRANSFER_BIT, VulkanInstance::get()->getPhysicalDevice()) };
		uint32_t indicesSize = (uint32_t)(model.indices.size() * sizeof(uint32_t));
		uint32_t verticesSize = (uint32_t)(model.vertices.size() * sizeof(Vertex));

		stagingBuffers->geometryBuffer.init(verticesSize + indicesSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, queueIndices);
		stagingBuffers->geometryMemory.bindBuffer(&stagingBuffers->geometryBuffer);
	}
}

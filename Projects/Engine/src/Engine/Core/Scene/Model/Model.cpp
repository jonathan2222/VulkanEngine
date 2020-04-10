#include "stdafx.h"
#include "Model.h"

namespace ym
{
	Model::Model()
	{
		static uint32_t idGenerator = 0;
		this->uniqueId = idGenerator++;
	}

	Model::~Model()
	{

	}

	void Model::destroy()
	{
		for (Node& node : this->nodes)
			destroyNode(node);

		if (this->indices.empty() == false)
			this->indexBuffer.destroy();
		if (this->vertices.empty() == false)
			this->vertexBuffer.destroy();

		if (this->indices.empty() == false && this->vertices.empty() == false)
			this->bufferMemory.destroy();

		if (hasImageMemory)
			this->imageMemory.destroy();
		if (hasMaterialMemory)
			this->materialMemory.destroy();

		for (Texture& texture : this->textures)
			texture.destroy();
		imageData.clear();

		for (Sampler& sampler : this->samplers)
			sampler.destroy();

		this->hasLoaded = false;
	}

	void Model::destroyNode(Node& node)
	{
		for (UniformBuffer& ub : node.uniformBuffers)
			ub.destroy();
		for (Node& child : node.children)
			destroyNode(child);
	}
}
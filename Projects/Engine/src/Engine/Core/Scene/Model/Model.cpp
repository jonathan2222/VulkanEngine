#include "stdafx.h"
#include "Model.h"

namespace ym
{
	Model::Model()
	{

	}

	Model::~Model()
	{

	}

	void Model::destroy()
	{
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
	}
}
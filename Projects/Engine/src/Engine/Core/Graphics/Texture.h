#pragma once

#include "ResourceManager.h"
#include "Types.h"

namespace ym
{
	class Texture
	{
	public:
		Texture();
		virtual ~Texture();

		static Texture* create();

		virtual void setData(ResourceManager::Image* img, Format format, Sampler sampler, Usage usage) = 0;
		virtual void* getBuffer() = 0;

		virtual ResourceManager::Image getImageInfo() = 0;
		virtual Format getFormat() = 0;
		virtual Sampler getSampler() = 0;
	};
}
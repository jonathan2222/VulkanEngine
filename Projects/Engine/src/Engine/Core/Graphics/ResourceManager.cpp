#include "stdafx.h"
#include "ResourceManager.h"

#pragma warning( push )
#pragma warning( disable : 6011 )
#pragma warning( disable : 6262 )
#pragma warning( disable : 6308 )
#pragma warning( disable : 6387 )
#pragma warning( disable : 26451 )
#pragma warning( disable : 28182 )
#define STB_IMAGE_IMPLEMENTATION
#include "../../../Utils/stb_image.h"
#pragma warning( pop )

ym::ResourceManager::ResourceManager()
{
	stbi_set_flip_vertically_on_load((int)true);
}

ym::ResourceManager& ym::ResourceManager::get()
{
	static ResourceManager resourceManager;
	return resourceManager;
}

ym::ResourceManager::Image* ym::ResourceManager::loadImage(const std::string& fileName, int nChannels)
{
	YM_PROFILER_FUNCTION();

	std::string path = std::string(YM_TEXTURE_PATH) + fileName;
	Image* img = new Image();
	int width = 0, height = 0, channelCount = 0;
	if (nChannels < 0 || nChannels > 4)
	{
		img->data = nullptr;
		YM_LOG_WARN("Unable to load texture [{0}]: Requested number of channels is not supported! Requested {1}.", path.c_str(), nChannels);
	}
	else
	{
		YM_PROFILER_SCOPE("stbi_load");
		img->data = (void*)stbi_load(path.c_str(), &width, &height, &channelCount, nChannels);
		img->format = getFormatFromChannelCount(nChannels == 0 ? channelCount : nChannels);
	}

	img->width = (unsigned int)width;
	img->height = (unsigned int)height;
	img->nChannels = (unsigned int)nChannels;

	if (!img->data)
	{
		YM_LOG_WARN("Unable to load texture [{0}]: file not found!", path.c_str());
	}

	return img;
}

void ym::ResourceManager::freeImage(Image* image)
{
	if (image)
	{
		if (image->data)
			stbi_image_free(image->data);
		delete image;
	}
	else
		YM_LOG_WARN("Trying to free an image pointer which is NULL!");
}

#define YM_CLAMP(x, a, b) ((x) < (a) ? (a) : ((x) > (b) ? (b) : (x)))

void ym::ResourceManager::convertFormat(Image* img, Format newFormat) const
{
	YM_PROFILER_FUNCTION();

	unsigned int w = img->width;
	unsigned int h = img->height;
	unsigned int size = w * h;
	if(img->format == Format::UINT_8_RGBA)
	{
		unsigned char* oldData = (unsigned char*)img->data;
		if (newFormat == Format::FLOAT_32_RGBA)
		{
			float* newData = new float[size*4u];
			for (unsigned int i = 0; i < size * 4u; i++)
				newData[i] = YM_CLAMP((float)oldData[i]/255.f, 0.f, 1.f);
			delete[] oldData;
			img->data = newData;
		}
		else if (newFormat == Format::SINT_32_RGBA)
		{
			YM_LOG_WARN("Conversion is not implemented!");
		}
		else if (newFormat == Format::UINT_32_RGBA)
		{
			YM_LOG_WARN("Conversion is not implemented!");
		}
		else if (newFormat == Format::SINT_8_RGBA)
		{
			YM_LOG_WARN("Conversion is not implemented!");
		}
	}
	else
	{
		YM_LOG_WARN("Conversion is not implemented!");
	}
}

ym::Format ym::ResourceManager::getFormatFromChannelCount(int nChannels) const
{
	switch (nChannels)
	{
	case 1: return Format::UINT_8_R; break;
	case 2: return Format::UINT_8_RG; break;
	case 3: return Format::UINT_8_RGB; break;
	case 4:
	default:
		return Format::UINT_8_RGBA; break;
	}
}

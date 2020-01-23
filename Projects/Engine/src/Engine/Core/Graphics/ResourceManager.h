#pragma once

#include <string>

#include "Types.h"

#define YM_TEXTURE_PATH "./Resources/Textures/"

namespace ym
{
	class ResourceManager
	{
	public:
		static ResourceManager& get();

		struct Image
		{
			void* data;
			Format format;
			unsigned int width;
			unsigned int height;
			unsigned int nChannels;
		};

		/*
			Load a image from memory.
			Arguments:
				* fileName: The path to the file, including the name of the image file with its extention.
				* nChannels: How many channels it will use. It can be 0, 1, 2, 3 or 4. 
					If it is set to 0, it will use the same number of channels as the image file on disk.
			Returns:
				A pointer to the image structure.
		*/
		Image* loadImage(const std::string& fileName, int nChannels);

		/*
			Free image data and deallocate the image structure.
			Arguments:
				* image: A pointer to the image structure.
		*/
		void freeImage(Image* image);

		void convertFormat(Image* img, Format newFormat) const;

	private:
		ResourceManager();

		Format getFormatFromChannelCount(int nChannels) const;

	};
}
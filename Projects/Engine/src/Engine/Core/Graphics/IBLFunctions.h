#pragma once

#include "Engine/Core/Vulkan/Texture.h"

namespace ym
{
	class IBLFunctions
	{
	public:
		/*
			Convert a 2D texture from equirectangular form to a cubemap form with the desired mip levels.
			If desiredMipLevels is 0, the final texture will use as many mip levels as it can.
		*/
		static Texture* convertEquirectangularToCubemap(uint32_t sideSize, Texture* texture, uint32_t desiredMipLevels);

		static std::pair<ym::Texture*, ym::Texture*> createIrradianceAndPrefilteredMap(Texture* environmentCube);

		static Texture* createBRDFLutTexture(uint32_t size);
	};
}
#include "stdafx.h"
#include "Texture.h"

#include "../../../Platform/GL/GLTexture.h"
#ifdef YAMI_PLATFORM_WINDOWS
#include "../../../Platform/DX11/DX11Texture.h"
#endif

ym::Texture::Texture()
{
}

ym::Texture::~Texture()
{
}

ym::Texture* ym::Texture::create()
{
	static std::string type = Config::get()->fetch<std::string>("API/type");
	if (type == YM_API_GL) return new GLTexture();
	else if (type == YM_API_DX11) return new DX11Texture();
	YM_ASSERT(false, "Could not create Texture: API not supported!");
	return nullptr;
}

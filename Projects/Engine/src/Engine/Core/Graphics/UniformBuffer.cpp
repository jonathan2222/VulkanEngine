#include "stdafx.h"
#include "UniformBuffer.h"

#include "../../../Platform/GL/GLUniformBuffer.h"
#ifdef YAMI_PLATFORM_WINDOWS
#include "../../../Platform/DX11/DX11UniformBuffer.h"
#endif

ym::UniformBuffer::UniformBuffer()
{
}

ym::UniformBuffer::~UniformBuffer()
{
}

ym::UniformBuffer* ym::UniformBuffer::create()
{
	static std::string type = Config::get()->fetch<std::string>("API/type");
	if (type == YM_API_GL) return new GLUniformBuffer();
	else if (type == YM_API_DX11) return new DX11UniformBuffer();
	YM_ASSERT(false, "Could not create UniformBuffer: API not supported!");
	return nullptr;
}

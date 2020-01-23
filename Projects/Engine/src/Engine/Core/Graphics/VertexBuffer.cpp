#include "stdafx.h"
#include "VertexBuffer.h"

#include "../../../Platform/GL/GLVertexBuffer.h"
#ifdef YAMI_PLATFORM_WINDOWS
#include "../../../Platform/DX11/DX11VertexBuffer.h"
#endif

ym::VertexBuffer::VertexBuffer()
{
}

ym::VertexBuffer::~VertexBuffer()
{
}

ym::VertexBuffer* ym::VertexBuffer::create()
{
	static std::string type = Config::get()->fetch<std::string>("API/type");
	if (type == YM_API_GL) return new GLVertexBuffer();
	else if (type == YM_API_DX11) return new DX11VertexBuffer();
	YM_ASSERT(false, "Could not create VertexBuffer: API not supported!");
	return nullptr;
}

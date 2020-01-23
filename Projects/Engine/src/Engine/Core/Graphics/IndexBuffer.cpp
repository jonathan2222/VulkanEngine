#include "stdafx.h"
#include "IndexBuffer.h"

#include "../../../Platform/GL/GLIndexBuffer.h"
#ifdef YAMI_PLATFORM_WINDOWS
#include "../../../Platform/DX11/DX11IndexBuffer.h"
#endif

ym::IndexBuffer::IndexBuffer() : m_count(0)
{
}

ym::IndexBuffer::~IndexBuffer()
{
}

ym::IndexBuffer* ym::IndexBuffer::create()
{
	static std::string type = Config::get()->fetch<std::string>("API/type");
	if (type == YM_API_GL) return new GLIndexBuffer();
	else if (type == YM_API_DX11) return new DX11IndexBuffer();
	YM_ASSERT(false, "Could not create IndexBuffer: API not supported!");
	return nullptr;
}

unsigned int ym::IndexBuffer::getCount() const
{
	return m_count;
}

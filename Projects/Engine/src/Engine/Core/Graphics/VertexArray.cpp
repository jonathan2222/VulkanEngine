#include "stdafx.h"
#include "VertexArray.h"

#include "../../../Platform/GL/GLVertexArray.h"
#ifdef YAMI_PLATFORM_WINDOWS
#include "../../../Platform/DX11/DX11VertexArray.h"
#endif

ym::VertexArray::VertexArray()
{
}

ym::VertexArray::~VertexArray()
{
}

ym::VertexArray* ym::VertexArray::create()
{
	static std::string type = Config::get()->fetch<std::string>("API/type");
	if (type == YM_API_GL) return new GLVertexArray();
	else if (type == YM_API_DX11) return new DX11VertexArray();
	YM_ASSERT(false, "Could not create VertexArray: API not supported!");
	return nullptr;
}

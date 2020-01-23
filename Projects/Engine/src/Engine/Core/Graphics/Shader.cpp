#include "stdafx.h"
#include "Shader.h"

#include "../../../Platform/GL/GLShader.h"
#ifdef YAMI_PLATFORM_WINDOWS
#include "../../../Platform/DX11/DX11Shader.h"
#endif

ym::Shader::Shader()
{
}

ym::Shader::~Shader()
{
}

ym::Shader* ym::Shader::create()
{
	static std::string type = Config::get()->fetch<std::string>("API/type");
	if (type == YM_API_GL) return new GLShader();
	else if (type == YM_API_DX11) return new DX11Shader();
	YM_ASSERT(false, "Could not create Shader: API not supported!");
	return nullptr;
}

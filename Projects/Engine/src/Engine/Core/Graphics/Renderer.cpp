#include "stdafx.h"
#include "Renderer.h"

#include "../../../Platform/GL/GLRenderer.h"
#ifdef YAMI_PLATFORM_WINDOWS
	#include "../../../Platform/DX11/DX11Renderer.h"
#endif

ym::Renderer::Renderer()
{
	deactivate();
}

ym::Renderer::~Renderer()
{
}

ym::Renderer* ym::Renderer::get()
{
	static std::string type = Config::get()->fetch<std::string>("API/type");
	if (type == YM_API_GL) return GLRenderer::get();
	else if (type == YM_API_DX11) return DX11Renderer::get();
	YM_ASSERT(false, "Could not fetch Renderer: API not supported!");
	return nullptr;
}

bool ym::Renderer::isActive() const
{
	return m_isActive;
}

void ym::Renderer::activate()
{
	m_isActive = true;
}

void ym::Renderer::deactivate()
{
	m_isActive = false;
}

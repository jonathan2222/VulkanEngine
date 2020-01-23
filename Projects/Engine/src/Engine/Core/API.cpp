#include "stdafx.h"
#include "API.h"

#include "../../Platform/GL/GLAPI.h"
#ifdef YAMI_PLATFORM_WINDOWS
	#include "../../Platform/DX11/DX11API.h"
#endif

ym::API* ym::API::get()
{
	static std::string type = Config::get()->fetch<std::string>("API/type");
	if (type == YM_API_GL) return GLAPI::get();
	else if (type == YM_API_DX11) return DX11API::get();
	YM_ASSERT(false, "Could not fetch API: API not supported!");
	return nullptr;
}

ym::API::VideoCardInfo& ym::API::getVideoCardInfo()
{
	return m_videoCardinfo;
}

#include "stdafx.h"
#include "API.h"

ym::API* ym::API::get()
{
	static std::string type = Config::get()->fetch<std::string>("API/type");
	YM_ASSERT(false, "Could not fetch API: API not supported!");
	return nullptr;
}

ym::API::VideoCardInfo& ym::API::getVideoCardInfo()
{
	return m_videoCardinfo;
}

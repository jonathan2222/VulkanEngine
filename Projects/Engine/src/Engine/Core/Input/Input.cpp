#include "stdafx.h"
#include "Input.h"

ym::Input* ym::Input::m_self = nullptr;

ym::Input* ym::Input::get()
{
	return m_self;
}

ym::Input* ym::Input::create()
{
	if (m_self != nullptr) delete m_self;
	m_self = nullptr;

	// Convert keys to match the API.
	KeyConverter::init();

	static std::string type = Config::get()->fetch<std::string>("API/type");
	YM_ASSERT(false, "Could not create Input: API not supported!");

	return m_self;
}
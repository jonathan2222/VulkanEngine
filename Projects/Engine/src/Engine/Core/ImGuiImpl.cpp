#include "stdafx.h"
#include "ImGuiImpl.h"

ym::ImGuiImpl* ym::ImGuiImpl::create()
{
	static std::string type = Config::get()->fetch<std::string>("API/type");
	YM_ASSERT(false, "Could not create ImGuiImpl: API not supported!");
	return nullptr;
}

bool ym::ImGuiImpl::isActive()
{
	return m_isActive;
}

void ym::ImGuiImpl::activate()
{
	m_isActive = true;
}

void ym::ImGuiImpl::deactivate()
{
	m_isActive = false;
}

bool ym::ImGuiImpl::needInput()
{
	if (isActive())
		return ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
	else return false;
}

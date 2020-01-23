#include "stdafx.h"
#include "Display.h"

#include "../../Platform/GL/GLDisplay.h"
#ifdef YAMI_PLATFORM_WINDOWS
	#include "../../Platform/DX11/DX11Display.h"
#endif

ym::DisplayDesc::DisplayDesc() : width(0), height(0), title(""), fullscreen(false), vsync(false), refreshRateNumerator(0), refreshRateDenominator(0)
{
}

void ym::DisplayDesc::init()
{
	YM_PROFILER_FUNCTION();
	if(this->width == 0)
		this->width = Config::get()->fetch<int>("Display/defaultWidth");
	if (this->height == 0)
		this->height = Config::get()->fetch<int>("Display/defaultHeight");
	if(this->title.empty())
		this->title = Config::get()->fetch<std::string>("Display/title");
	if(this->fullscreen == false)
		this->fullscreen = Config::get()->fetch<bool>("Display/fullscreen");
	if (this->vsync == false)
		this->vsync = Config::get()->fetch<bool>("Display/vsync");
}

void ym::DisplayDesc::copy(const DisplayDesc& other)
{
	this->title = other.title;
	this->width = other.width;
	this->height = other.height;
	this->refreshRateDenominator = other.refreshRateDenominator;
	this->refreshRateNumerator = other.refreshRateNumerator;
	this->fullscreen = other.fullscreen;
	this->vsync = other.vsync;
}

ym::Display* ym::Display::m_self = nullptr;

ym::Display* ym::Display::create(const DisplayDesc& description)
{
	if (m_self != nullptr) delete m_self;

	static std::string type = Config::get()->fetch<std::string>("API/type");
	if (type == YM_API_GL) m_self = new GLDisplay(description);
	else if (type == YM_API_DX11) m_self = new DX11Display(description);

	return m_self;
}

void ym::Display::setDescription(const DisplayDesc& description)
{
	m_description.copy(description);
}

ym::DisplayDesc& ym::Display::getDescription()
{
	return m_description;
}

int ym::Display::getWidth() const
{
	return m_description.width;
}

int ym::Display::getHeight() const
{
	return m_description.height;
}

float ym::Display::getAspectRatio() const
{
	return (float)getWidth()/getHeight();
}

void ym::Display::setImGuiImpl(ImGuiImpl* imGuiImpl)
{
	m_imGuiImpl = imGuiImpl;
}

ym::ImGuiImpl* ym::Display::getImGuiImpl()
{
	return m_imGuiImpl;
}

ym::Display* ym::Display::get()
{
	return m_self;
}

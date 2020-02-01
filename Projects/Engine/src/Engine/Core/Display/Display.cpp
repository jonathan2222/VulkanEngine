#include "stdafx.h"
#include "Display.h"

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

ym::Display::~Display()
{
}


void ym::Display::init()
{
	YM_PROFILER_FUNCTION();

	this->shouldCloseV = false;

	GLFWmonitor* monitor = nullptr;
	if (this->description.fullscreen)
	{
		monitor = glfwGetPrimaryMonitor();

		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
		this->description.width = mode->width;
		this->description.height = mode->height;
	}

	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	this->window = glfwCreateWindow(this->description.width, this->description.height, this->description.title.c_str(), monitor, nullptr);
	glfwMakeContextCurrent(this->window);
	//glfwSetFramebufferSizeCallback(m_window, GLDisplay::frameBufferSizeCallback);
}

void ym::Display::destroy()
{
	// Destroy the current window.
	glfwDestroyWindow(this->window);
}

void ym::Display::setDescription(const DisplayDesc& description)
{
	this->description.copy(description);
}

ym::DisplayDesc& ym::Display::getDescription()
{
	return this->description;
}

bool ym::Display::shouldClose() const
{
	return glfwWindowShouldClose(this->window) != 0 || this->shouldCloseV;
}

void ym::Display::close() noexcept
{
	this->shouldCloseV = true;
}

void ym::Display::pollEvents()
{
	// Fetch window events.
	glfwPollEvents();
}

GLFWwindow* ym::Display::getWindowPtr()
{
	return this->window;
}

int ym::Display::getWidth() const
{
	return this->description.width;
}

int ym::Display::getHeight() const
{
	return this->description.height;
}

float ym::Display::getAspectRatio() const
{
	return (float)getWidth()/getHeight();
}

void ym::Display::frameBufferSizeCallback(GLFWwindow* window, int width, int height)
{
}

ym::Display* ym::Display::get()
{
	static Display display;
	return &display;
}

#include "stdafx.h"
#include "Renderer.h"

#include "../Input/Config.h"

ym::Renderer::Renderer()
{
	this->instance = VulkanInstance::get();
}

ym::Renderer::~Renderer()
{
}

void ym::Renderer::init()
{
	this->instance->init();

	int width = Config::get()->fetch<int>("Display/defaultWidth");
	int height = Config::get()->fetch<int>("Display/defaultHeight");
	this->swapChain.init(width, height);
}

void ym::Renderer::destroy()
{
	this->swapChain.destory();

	this->instance->destroy();
}

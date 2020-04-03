#include "stdafx.h"
#include "ModelRenderer.h"

ym::ModelRenderer::ModelRenderer()
{
	this->instance = VulkanInstance::get();
}

ym::ModelRenderer::~ModelRenderer()
{
}

void ym::ModelRenderer::init()
{
	this->instance->init();

	int width = Config::get()->fetch<int>("Display/defaultWidth");
	int height = Config::get()->fetch<int>("Display/defaultHeight");
	this->swapChain.init(width, height);
}

void ym::ModelRenderer::destroy()
{
	this->swapChain.destory();
	this->instance->destroy();
}

#include "stdafx.h"
#include "LayerManager.h"

#include "Layer.h"
#include "App.h"

ym::LayerManager* ym::LayerManager::get()
{
	static LayerManager manager;
	return &manager;
}

void ym::LayerManager::setApp(App* app)
{
	this->appPtr = app;
}

ym::LayerManager::~LayerManager()
{
	for (Layer*& layer : this->layers)
		delete layer;
	this->layers.clear();
}

void ym::LayerManager::push(Layer* layer)
{
	YM_PROFILER_FUNCTION();
	this->layers.push_back(layer);
}

void ym::LayerManager::pop()
{
	YM_PROFILER_FUNCTION();
	Layer* layer = this->layers.back();
	layer->onQuit();
	delete layer;
	this->layers.pop_back();
}

void ym::LayerManager::onStart(Renderer* renderer)
{
	for (Layer*& layer : this->layers)
		layer->onStart(renderer);
}

void ym::LayerManager::onUpdate(float dt)
{
	YM_PROFILER_RENDERING_FUNCTION();
	// Update the active layer
	this->layers.back()->onUpdate(dt);
}

void ym::LayerManager::onRender(Renderer* renderer)
{
	YM_PROFILER_RENDERING_FUNCTION();
	// Render the active layer
	this->layers.back()->onRender(renderer);
}

void ym::LayerManager::onRenderImGui()
{
	YM_PROFILER_RENDERING_FUNCTION();
	// Render imGui on the active layer
	this->layers.back()->onRenderImGui();
}

void ym::LayerManager::onQuit()
{
	for (Layer*& layer : this->layers)
		layer->onQuit();
}

void ym::LayerManager::terminate()
{
	this->appPtr->terminate();
}

ym::CommandPools* ym::LayerManager::getCommandPools()
{
	return &this->appPtr->commandPools;
}

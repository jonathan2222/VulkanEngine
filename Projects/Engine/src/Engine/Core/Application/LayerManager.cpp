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

void ym::LayerManager::onStart()
{
	for (Layer*& layer : this->layers)
		layer->onStart();
}

void ym::LayerManager::onUpdate(float dt)
{
	YM_PROFILER_RENDERING_FUNCTION();
	// Update the active layer
	this->layers.back()->onUpdate(dt);
}

void ym::LayerManager::onRender()
{
	YM_PROFILER_RENDERING_FUNCTION();
	// Render the active layer
	this->layers.back()->onRender();
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

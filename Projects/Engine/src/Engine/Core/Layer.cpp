#include "stdafx.h"
#include "Layer.h"

ym::Layer::Layer() : m_api(nullptr), m_input(nullptr), m_display(nullptr)
{
}

ym::Layer::~Layer()
{
}

ym::API* ym::Layer::getAPI()
{
	return m_api;
}

ym::Input* ym::Layer::getInput()
{
	return m_input;
}

ym::Display* ym::Layer::getDisplay()
{
	return m_display;
}

ym::LayerManager* ym::Layer::getManager()
{
	return LayerManager::get();
}

void ym::Layer::init(API* api, Input* input, Display* display)
{
	m_api = api;
	m_input = input;
	m_display = display;
}

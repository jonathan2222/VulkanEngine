#include "stdafx.h"
#include "Layer.h"

ym::Layer::Layer()
{
}

ym::Layer::~Layer()
{
}

ym::LayerManager* ym::Layer::getManager()
{
	return LayerManager::get();
}

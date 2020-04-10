#include "stdafx.h"
#include "Layer.h"

ym::Layer::Layer()
{
}

ym::Layer::~Layer()
{
}

void ym::Layer::terminate()
{
	getManager()->terminate();
}

ym::LayerManager* ym::Layer::getManager()
{
	return LayerManager::get();
}

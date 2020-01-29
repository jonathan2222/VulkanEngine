#include "Engine/Core/Application/App.h"
#include "Engine/EntryPoint.h"

#include "SandboxLayer.h"

class Application : public ym::App
{
public:
	Application()
	{

	}

	virtual ~Application()
	{

	}

	void start() override
	{
		this->layerManager->push(new SandboxLayer());
	}
};

ym::App* ym::createApplication()
{
	return new Application();
}

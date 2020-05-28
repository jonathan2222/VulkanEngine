#include "Engine/Core/Application/App.h"
#include "Engine/EntryPoint.h"

#include "SandboxLayer.h"
#include "TestLayer.h"

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
		//this->layerManager->push(new TestLayer());
	}
};

ym::App* ym::createApplication()
{
	return new Application();
}

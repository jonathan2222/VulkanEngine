#pragma once

#ifdef YM_DEBUG
	#include <crtdbg.h>
#endif

extern ym::App* ym::createApplication();

int main(int argc, char* argv[])
{
#ifdef YM_DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	ym::App* app = ym::createApplication();

	app->start();

	app->run();

	delete app;
}
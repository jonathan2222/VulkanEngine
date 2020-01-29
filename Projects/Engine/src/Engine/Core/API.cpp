#include "stdafx.h"
#include "API.h"

ym::API* ym::API::get()
{
	static API api;
	return &api;
}

void ym::API::init()
{
	// Initialize glfw.
	glfwInit();
	// Tell glfw to not create an OpenGL context.
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
}

void ym::API::destroy()
{
	// Destroy glfw.
	glfwTerminate();
}

ym::API::VideoCardInfo& ym::API::getVideoCardInfo()
{
	return this->videoCardinfo;
}

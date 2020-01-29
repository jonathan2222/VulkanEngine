#include "stdafx.h"
#include "Input.h"

#include <GLFW/glfw3.h>

#include "../Display/Display.h"

std::unordered_map<ym::Key, ym::KeyState> ym::Input::s_keyMap = std::unordered_map<ym::Key, ym::KeyState>();
std::unordered_map<ym::MB, ym::KeyState> ym::Input::s_mbMap = std::unordered_map<ym::MB, ym::KeyState>();
glm::vec2 ym::Input::s_mousePos = glm::vec2(0.0f);

ym::Input* ym::Input::get()
{
	static Input input;
	return &input;
}

void ym::Input::init()
{
	YM_PROFILER_FUNCTION();

	GLFWwindow* wnd = static_cast<GLFWwindow*>(Display::get()->getNativeDisplay());
	glfwSetKeyCallback(wnd, Input::keyCallback);
	glfwSetCursorPosCallback(wnd, Input::cursorPositionCallback);
	glfwSetMouseButtonCallback(wnd, Input::mouseButtonCallback);

	for (int i = (int)Key::FIRST; i <= (int)Key::LAST; i++)
		s_keyMap[(Key)i] = KeyState::RELEASED;

	for (int i = (int)MB::LEFT; i <= (int)MB::MIDDLE; i++)
		s_mbMap[(MB)i] = KeyState::RELEASED;
}

bool ym::Input::isKeyPressed(const Key& key) const
{
	return s_keyMap[key] == KeyState::PRESSED;
}

bool ym::Input::isKeyReleased(const Key& key) const
{
	return s_keyMap[key] == KeyState::RELEASED;
}

glm::vec2 ym::Input::getMousePos() const
{
	return s_mousePos;
}

bool ym::Input::isMBPressed(const MB& button) const
{
	return s_mbMap[button] == KeyState::PRESSED;
}

bool ym::Input::isMBReleased(const MB& button) const
{
	return s_mbMap[button] == KeyState::RELEASED;
}

void ym::Input::lockMouse() const
{
	GLFWwindow* wnd = static_cast<GLFWwindow*>(Display::get()->getNativeDisplay());
	glfwSetInputMode(wnd, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void ym::Input::unlockMouse() const
{
	GLFWwindow* wnd = static_cast<GLFWwindow*>(Display::get()->getNativeDisplay());
	glfwSetInputMode(wnd, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void ym::Input::keyCallback(GLFWwindow* wnd, int key, int scancode, int action, int mods)
{
	//ImGuiImpl* imGuiImpl = Display::get()->getImGuiImpl();
	//if (imGuiImpl == nullptr || (imGuiImpl != nullptr && imGuiImpl->needInput() == false))
	{
		Key ymKey = (Key)key;
		if (action == GLFW_PRESS)
			s_keyMap[ymKey] = KeyState::PRESSED;
		if (action == GLFW_RELEASE)
			s_keyMap[ymKey] = KeyState::RELEASED;
	}
}

void ym::Input::cursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
	//ImGuiImpl* imGuiImpl = Display::get()->getImGuiImpl();
	//if (imGuiImpl == nullptr || (imGuiImpl != nullptr && imGuiImpl->needInput() == false))
	{
		s_mousePos.x = (float)xpos;
		s_mousePos.y = (float)ypos;
	}
}

void ym::Input::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	//ImGuiImpl* imGuiImpl = Display::get()->getImGuiImpl();
	//if (imGuiImpl == nullptr || (imGuiImpl != nullptr && imGuiImpl->needInput() == false))
	{
		MB ymButton = (MB)button;
		if (action == GLFW_PRESS)
			s_mbMap[ymButton] = KeyState::PRESSED;
		if (action == GLFW_RELEASE)
			s_mbMap[ymButton] = KeyState::RELEASED;
	}
}

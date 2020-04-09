#include "stdafx.h"
#include "Input.h"

#include <GLFW/glfw3.h>

#include "../Display/Display.h"

std::unordered_map<ym::Key, ym::KeyState> ym::Input::s_keyMap = std::unordered_map<ym::Key, ym::KeyState>();
std::unordered_map<ym::MB, ym::KeyState> ym::Input::s_mbMap = std::unordered_map<ym::MB, ym::KeyState>();
glm::vec2 ym::Input::s_mousePos = glm::vec2(0.0f);
glm::vec2 ym::Input::s_mousePosPre = glm::vec2(0.0f);
glm::vec2 ym::Input::s_mouseDelta = glm::vec2(0.0f);
glm::vec2 ym::Input::s_scrollDelta = glm::vec2(0.0f);

ym::Input* ym::Input::get()
{
	static Input input;
	return &input;
}

void ym::Input::init()
{
	YM_PROFILER_FUNCTION();

	GLFWwindow* wnd = static_cast<GLFWwindow*>(Display::get()->getWindowPtr());
	glfwSetKeyCallback(wnd, Input::keyCallback);
	glfwSetCursorPosCallback(wnd, Input::cursorPositionCallback);
	glfwSetMouseButtonCallback(wnd, Input::mouseButtonCallback);
	glfwSetScrollCallback(wnd, Input::mouseScrollCallback);

	for (int i = (int)Key::FIRST; i <= (int)Key::LAST; i++)
		s_keyMap[(Key)i] = KeyState::RELEASED;

	for (int i = (int)MB::LEFT; i <= (int)MB::MIDDLE; i++)
		s_mbMap[(MB)i] = KeyState::RELEASED;
}

void ym::Input::update()
{
	for (auto& key : s_keyMap)
	{
		if (key.second == KeyState::FIRST_PRESSED)
			key.second = KeyState::PRESSED;
		if (key.second == KeyState::FIRST_RELEASED)
			key.second = KeyState::RELEASED;
	}

	s_mouseDelta = s_mousePos - s_mousePosPre;
	s_mousePosPre = s_mousePos;
}

bool ym::Input::isKeyPressed(const Key& key) const
{
	return s_keyMap[key] == KeyState::FIRST_PRESSED || s_keyMap[key] == KeyState::PRESSED;
}

bool ym::Input::isKeyReleased(const Key& key) const
{
	return !isKeyPressed(key);
}

ym::KeyState ym::Input::getKeyState(const Key& key)
{
	auto& it = s_keyMap.find(key);
	if (it == s_keyMap.end())
		return KeyState::RELEASED;
	else return it->second;
}

glm::vec2 ym::Input::getCursorDelta()
{
	return s_mouseDelta;
}

glm::vec2 ym::Input::getScrollDelta()
{
	return s_scrollDelta;
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

void ym::Input::centerMouse() const
{
	GLFWwindow* wnd = static_cast<GLFWwindow*>(Display::get()->getWindowPtr());
	s_mouseDelta = s_mousePos - s_mousePosPre;
	s_mousePos.x = (float)Display::get()->getWidth() * 0.5f;
	s_mousePos.y = (float)Display::get()->getHeight() * 0.5f;
	glfwSetCursorPos(wnd, (double)s_mousePos.x, (double)s_mousePos.y);
}

void ym::Input::lockMouse() const
{
	GLFWwindow* wnd = static_cast<GLFWwindow*>(Display::get()->getWindowPtr());
	glfwSetInputMode(wnd, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	s_mousePosPre = s_mousePos;
	s_mouseDelta.x = 0.0f;
	s_mouseDelta.y = 0.0f;
}

void ym::Input::unlockMouse() const
{
	GLFWwindow* wnd = static_cast<GLFWwindow*>(Display::get()->getWindowPtr());
	glfwSetInputMode(wnd, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	s_mousePosPre = s_mousePos;
	s_mouseDelta.x = 0.0f;
	s_mouseDelta.y = 0.0f;
}

std::string ym::Input::keyStateToStr(KeyState state)
{
	std::string str;
	switch (state)
	{
	case KeyState::RELEASED:
		str = "RELEASED";
		break;
	case KeyState::FIRST_RELEASED:
		str = "FIRST_RELEASED";
		break;
	case KeyState::PRESSED:
		str = "PRESSED";
		break;
	case KeyState::FIRST_PRESSED:
		str = "FIRST_PRESSED";
		break;
	default:
		str = "UNKNOWN_STATE";
		break;
	}
	return str;
}

void ym::Input::keyCallback(GLFWwindow* wnd, int key, int scancode, int action, int mods)
{
	//ImGuiImpl* imGuiImpl = Display::get()->getImGuiImpl();
	//if (imGuiImpl == nullptr || (imGuiImpl != nullptr && imGuiImpl->needInput() == false))
	{
		Key ymKey = (Key)key;
		if (action == GLFW_PRESS)
			s_keyMap[ymKey] = KeyState::FIRST_PRESSED;
		if (action == GLFW_RELEASE)
			s_keyMap[ymKey] = KeyState::FIRST_RELEASED;
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

void ym::Input::mouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
	s_scrollDelta.x = (float)xOffset;
	s_scrollDelta.y = (float)yOffset;
}

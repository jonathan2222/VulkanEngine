#pragma once

#include "Keys.h"
#include "glm/vec2.hpp"
#include <unordered_map>

struct GLFWwindow;
namespace ym
{
	class Input
	{
	public:
		static Input* get();

		Input() = default;
		virtual ~Input() = default;

		void init();
		void update();

		bool isKeyPressed(const Key& key) const;
		bool isKeyReleased(const Key& key) const;
		KeyState getKeyState(const Key& key);

		glm::vec2 getCursorDelta();
		glm::vec2 getScrollDelta();
		glm::vec2 getMousePos() const;
		bool isMBPressed(const MB& button) const;
		bool isMBReleased(const MB& button) const;

		void centerMouse() const;
		void lockMouse() const;
		void unlockMouse() const;

		static std::string keyStateToStr(KeyState state);

	private:
		static void keyCallback(GLFWwindow* wnd, int key, int scancode, int action, int mods);
		static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
		static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
		static void mouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset);

	private:
		static std::unordered_map<Key, KeyState> s_keyMap;
		static std::unordered_map<MB, KeyState> s_mbMap;
		static glm::vec2 s_mousePos;
		static glm::vec2 s_mousePosPre;
		static glm::vec2 s_mouseDelta;
		static glm::vec2 s_scrollDelta;
	};
}

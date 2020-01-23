#pragma once

#include "Keys.h"
#include "glm/vec2.hpp"

namespace ym
{
	class Input
	{
	public:
		static Input* get();
		static Input* create();

		Input() = default;
		virtual ~Input() = default;

		virtual void init() = 0;
		virtual bool isKeyPressed(const Key& key) const = 0;
		virtual bool isKeyReleased(const Key& key) const = 0;

		virtual glm::vec2 getMousePos() const = 0;
		virtual bool isMBPressed(const MB& button) const = 0;
		virtual bool isMBReleased(const MB& button) const = 0;

		virtual void lockMouse() const = 0;
		virtual void unlockMouse() const = 0;

	private:
		static Input* m_self;
	};
}

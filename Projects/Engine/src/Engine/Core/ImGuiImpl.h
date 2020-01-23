#pragma once

#include "../../Utils/Imgui/imgui.h"

namespace ym
{
	class ImGuiImpl
	{
	public:
		ImGuiImpl() = default;
		static ImGuiImpl* create();

		virtual void setUp() = 0;
		virtual void startFrame() = 0;
		virtual void endFrame() = 0;
		virtual void cleanUp() = 0;

		bool isActive();
		void activate();
		void deactivate();

		bool needInput();

	private:
		bool m_isActive = false;
		bool m_needInput = false;
	};
}
#pragma once

#include <string>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace ym
{
	class API
	{
	public:
		static API* get();

		void init();
		void destroy();

		struct VideoCardInfo
		{
			std::string name;
			int videoMemory;
			int systemMemory;
			int sharedSystemMemory;
		};

		VideoCardInfo& getVideoCardInfo();

	private:
		VideoCardInfo videoCardinfo;
	};
}

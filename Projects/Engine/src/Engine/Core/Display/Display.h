#pragma once

#include <string>

#include "../API.h"

namespace ym
{
	struct DisplayDesc
	{
		std::string title;
		int width;
		int height;
		unsigned int refreshRateNumerator;
		unsigned int refreshRateDenominator;
		bool fullscreen;
		bool vsync;

		DisplayDesc();
		DisplayDesc(const std::string& title) :
			width(0), height(0), title(title), fullscreen(true), vsync(false), refreshRateNumerator(0), refreshRateDenominator(0) {}
		DisplayDesc(int width, int height, const std::string& title) :
			width(width), height(height), title(title), fullscreen(false), vsync(false), refreshRateNumerator(0), refreshRateDenominator(0) {}
		void init();
		void copy(const DisplayDesc& other);
	};

	class Display
	{
	public:
		static Display* get();

		virtual ~Display();

		void init();
		void destroy();

		void setDescription(const DisplayDesc& description);
		DisplayDesc& getDescription();

		bool shouldClose() const;
		void close() noexcept;

		void pollEvents();

		void* getNativeDisplay();

		int getWidth() const;
		int getHeight() const;
		float getAspectRatio() const;

	private:
		static void frameBufferSizeCallback(GLFWwindow* window, int width, int height);

		bool shouldCloseV;
		GLFWwindow* window;
		DisplayDesc description;
	};
};

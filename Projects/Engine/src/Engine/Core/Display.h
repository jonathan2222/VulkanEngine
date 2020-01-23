#pragma once

#include <string>

struct GLFWwindow;

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

	class ImGuiImpl;
	class Display
	{
	public:
		static Display* get();

		/*
			Creates the display. This is implemented in another class.
		*/
		static Display* create(const DisplayDesc& description);
		virtual ~Display() = default;

		void setDescription(const DisplayDesc& description);
		DisplayDesc& getDescription();

		virtual bool shouldClose() const noexcept = 0;
		virtual void close() noexcept = 0;

		virtual void pollEvents() noexcept = 0;

		virtual void* getNativeDisplay() = 0;

		int getWidth() const;
		int getHeight() const;
		float getAspectRatio() const;

		void setImGuiImpl(ImGuiImpl* imGuiImpl);
		ym::ImGuiImpl* getImGuiImpl();

	protected:
		DisplayDesc m_description;

	private:
		ym::ImGuiImpl* m_imGuiImpl = nullptr;
		static Display* m_self;
	};
};

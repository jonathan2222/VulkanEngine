#pragma once

#include <unordered_map>
#ifdef YM_PLATFORM_WINDOWS
	#include <Windows.h>
#endif

namespace ym
{
	enum class KeyState
	{
		PRESSED,
		RELEASED
	};

	enum class MB
	{
		LEFT = 0,
		RIGHT = 1,
		MIDDLE = 2
	};

	enum class Key
	{
		UNKNOWN = -1,

		/* Printable keys */
		SPACE			  = 32,
		COMMA             = 44,  /* , */
		MINUS             = 45,  /* - */
		PERIOD            = 46,  /* . */
		NUMBER0           = 48,
		NUMBER1           = 49,
		NUMBER2           = 50,
		NUMBER3           = 51,
		NUMBER4           = 52,
		NUMBER5           = 53,
		NUMBER6           = 54,
		NUMBER7           = 55,
		NUMBER8           = 56,
		NUMBER9           = 57,
		A                 = 65,
		B                 = 66,
		C                 = 67,
		D                 = 68,
		E                 = 69,
		F                 = 70,
		G                 = 71,
		H                 = 72,
		I                 = 73,
		J                 = 74,
		K                 = 75,
		L                 = 76,
		M                 = 77,
		N                 = 78,
		O                 = 79,
		P                 = 80,
		Q                 = 81,
		R                 = 82,
		S                 = 83,
		T                 = 84,
		U                 = 85,
		V                 = 86,
		W                 = 87,
		X                 = 88,
		Y                 = 89,
		Z                 = 90,

		/* Function keys */
		ESCAPE            = 256,
		ENTER             = 257,
		TAB               = 258,
		BACKSPACE         = 259,
		INSERT            = 260,
		DELETE_KEY        = 261,
		RIGHT             = 262,
		LEFT              = 263,
		DOWN              = 264,
		UP                = 265,
		PAGE_UP           = 266,
		PAGE_DOWN         = 267,
		HOME              = 268,
		END               = 269,
		CAPS_LOCK         = 280,
		SCROLL_LOCK       = 281,
		NUM_LOCK          = 282,
		PRINT_SCREEN      = 283,
		PAUSE             = 284,
		F1                = 290,
		F2                = 291,
		F3                = 292,
		F4                = 293,
		F5                = 294,
		F6                = 295,
		F7                = 296,
		F8                = 297,
		F9                = 298,
		F10               = 299,
		F11               = 300,
		F12               = 301,
		F13               = 302,
		F14               = 303,
		F15               = 304,
		F16               = 305,
		F17               = 306,
		F18               = 307,
		F19               = 308,
		F20               = 309,
		F21               = 310,
		F22               = 311,
		F23               = 312,
		F24               = 313,
		KP_0              = 320,
		KP_1              = 321,
		KP_2              = 322,
		KP_3              = 323,
		KP_4              = 324,
		KP_5              = 325,
		KP_6              = 326,
		KP_7              = 327,
		KP_8              = 328,
		KP_9              = 329,
		KP_DECIMAL        = 330,
		KP_DIVIDE         = 331,
		KP_MULTIPLY       = 332,
		KP_SUBTRACT       = 333,
		KP_ADD            = 334,
		LEFT_SHIFT        = 340,
		LEFT_CONTROL      = 341,
		LEFT_ALT          = 342,
		RIGHT_SHIFT       = 344,
		RIGHT_CONTROL     = 345,
		RIGHT_ALT         = 346,
		MENU              = 348,

		FIRST = SPACE,
		LAST = MENU
	};

#ifdef YM_PLATFORM_WINDOWS
	class MBConverter
	{
	public:
		enum class DXMB
		{
			LEFT = VK_LBUTTON,
			RIGHT = VK_RBUTTON,
			MIDDLE = VK_MBUTTON
		};
		static MB convertToMB(DXMB mb);
	};

	class KeyConverter
	{
	public:
		enum class DXKey
		{
			UNKNOWN = -1,

			// Printable keys 
			BACKSPACE = VK_BACK,
			TAB = VK_TAB,
			ENTER = VK_RETURN,
			MENU = VK_MENU,	// ALT 
			PAUSE = VK_PAUSE,
			CAPS_LOCK = VK_CAPITAL, // Not supported yet
			ESCAPE = VK_ESCAPE,
			SPACE = VK_SPACE,
			PAGE_UP = VK_PRIOR,
			PAGE_DOWN = VK_NEXT,
			END = VK_END,
			HOME = VK_HOME,
			LEFT = VK_LEFT,
			UP = VK_UP,
			RIGHT = VK_RIGHT,
			DOWN = VK_DOWN,
			PRINT_SCREEN = VK_SNAPSHOT,
			INSERT = VK_INSERT,
			DELETE_KEY = VK_DELETE,
			NUMBER0 = 0x30,
			NUMBER1 = 0x31,
			NUMBER2 = 0x32,
			NUMBER3 = 0x33,
			NUMBER4 = 0x34,
			NUMBER5 = 0x35,
			NUMBER6 = 0x36,
			NUMBER7 = 0x37,
			NUMBER8 = 0x38,
			NUMBER9 = 0x39,
			A = 0x41,
			B = 0x42,
			C = 0x43,
			D = 0x44,
			E = 0x45,
			F = 0x46,
			G = 0x47,
			H = 0x48,
			I = 0x49,
			J = 0x4A,
			K = 0x4B,
			L = 0x4C,
			M = 0x4D,
			N = 0x4E,
			O = 0x4F,
			P = 0x50,
			Q = 0x51,
			R = 0x52,
			S = 0x53,
			T = 0x54,
			U = 0x55,
			V = 0x56,
			W = 0x57,
			X = 0x58,
			Y = 0x59,
			Z = 0x5A,
			KP_0 = VK_NUMPAD0,
			KP_1 = VK_NUMPAD1,
			KP_2 = VK_NUMPAD2,
			KP_3 = VK_NUMPAD3,
			KP_4 = VK_NUMPAD4,
			KP_5 = VK_NUMPAD5,
			KP_6 = VK_NUMPAD6,
			KP_7 = VK_NUMPAD7,
			KP_8 = VK_NUMPAD8,
			KP_9 = VK_NUMPAD9,
			KP_MULTIPLY = VK_MULTIPLY,
			KP_ADD = VK_ADD,
			KP_SUBTRACT = VK_SUBTRACT,
			KP_DECIMAL = VK_DECIMAL,
			KP_DIVIDE = VK_DIVIDE,
			F1 = VK_F1,
			F2 = VK_F2,
			F3 = VK_F3,
			F4 = VK_F4,
			F5 = VK_F5,
			F6 = VK_F6,
			F7 = VK_F7,
			F8 = VK_F8,
			F9 = VK_F9,
			F10 = VK_F10, // Not supported yet
			F11 = VK_F11,
			F12 = VK_F12,
			F13 = VK_F13,
			F14 = VK_F14,
			F15 = VK_F15,
			F16 = VK_F16,
			F17 = VK_F17,
			F18 = VK_F18,
			F19 = VK_F19,
			F20 = VK_F20,
			F21 = VK_F21,
			F22 = VK_F22,
			F23 = VK_F23,
			F24 = VK_F24,
			NUM_LOCK = VK_NUMLOCK, // Not supported yet
			SCROLL_LOCK = VK_SCROLL, // Not supported yet
			LEFT_SHIFT = VK_LSHIFT,
			RIGHT_SHIFT = VK_RSHIFT,
			LEFT_CONTROL = VK_LCONTROL,
			RIGHT_CONTROL = VK_RCONTROL,
			LEFT_ALT = VK_LMENU,
			RIGHT_ALT = VK_RMENU,
			COMMA = VK_OEM_COMMA,  // , 
			MINUS = VK_OEM_MINUS,  // - 
			PERIOD = VK_OEM_PERIOD,  // . 

			FIRST = BACKSPACE,
			LAST = PERIOD
		};

		static void init();
		static Key convertToKey(DXKey key);
	private:
		static std::unordered_map<DXKey, Key> m_keys;
	};
#endif
}
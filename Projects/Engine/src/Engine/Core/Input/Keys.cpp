#include "stdafx.h"
#include "Keys.h"

#ifdef YM_PLATFORM_WINDOWS
std::unordered_map<ym::KeyConverter::DXKey, ym::Key> ym::KeyConverter::m_keys = std::unordered_map<ym::KeyConverter::DXKey, ym::Key>();

void ym::KeyConverter::init()
{
	YM_PROFILER_FUNCTION();
	for (int i = (int)DXKey::FIRST; i <= (int)DXKey::LAST; i++)
	{
		Key value = Key::UNKNOWN;
		switch ((DXKey)i)
		{
		case DXKey::BACKSPACE:			value = Key::BACKSPACE; break;
		case DXKey::TAB:				value = Key::TAB; break;
		case DXKey::ENTER:				value = Key::ENTER; break;
		case DXKey::MENU:				value = Key::MENU; break;
		case DXKey::PAUSE:				value = Key::PAUSE; break;
		case DXKey::CAPS_LOCK:			value = Key::CAPS_LOCK; break;
		case DXKey::ESCAPE:				value = Key::ESCAPE; break;
		case DXKey::SPACE:				value = Key::SPACE; break;
		case DXKey::PAGE_UP:			value = Key::PAGE_UP; break;
		case DXKey::PAGE_DOWN:			value = Key::PAGE_DOWN; break;
		case DXKey::END:				value = Key::END; break;
		case DXKey::HOME:				value = Key::HOME; break;
		case DXKey::LEFT:				value = Key::LEFT; break;
		case DXKey::UP:					value = Key::UP; break;
		case DXKey::RIGHT:				value = Key::RIGHT; break;
		case DXKey::DOWN:				value = Key::DOWN; break;
		case DXKey::PRINT_SCREEN:		value = Key::PRINT_SCREEN; break;
		case DXKey::INSERT:				value = Key::INSERT; break;
		case DXKey::DELETE_KEY:			value = Key::DELETE_KEY; break;
		case DXKey::NUMBER0:			value = Key::NUMBER0; break;
		case DXKey::NUMBER1:			value = Key::NUMBER1; break;
		case DXKey::NUMBER2:			value = Key::NUMBER2; break;
		case DXKey::NUMBER3:			value = Key::NUMBER3; break;
		case DXKey::NUMBER4:			value = Key::NUMBER4; break;
		case DXKey::NUMBER5:			value = Key::NUMBER5; break;
		case DXKey::NUMBER6:			value = Key::NUMBER6; break;
		case DXKey::NUMBER7:			value = Key::NUMBER7; break;
		case DXKey::NUMBER8:			value = Key::NUMBER8; break;
		case DXKey::NUMBER9:			value = Key::NUMBER9; break;
		case DXKey::A:					value = Key::A; break;
		case DXKey::B:					value = Key::B; break;
		case DXKey::C:					value = Key::C; break;
		case DXKey::D:					value = Key::D; break;
		case DXKey::E:					value = Key::E; break;
		case DXKey::F:					value = Key::F; break;
		case DXKey::G:					value = Key::G; break;
		case DXKey::H:					value = Key::H; break;
		case DXKey::I:					value = Key::I; break;
		case DXKey::J:					value = Key::J; break;
		case DXKey::K:					value = Key::K; break;
		case DXKey::L:					value = Key::L; break;
		case DXKey::M:					value = Key::M; break;
		case DXKey::N:					value = Key::N; break;
		case DXKey::O:					value = Key::O; break;
		case DXKey::P:					value = Key::P; break;
		case DXKey::Q:					value = Key::Q; break;
		case DXKey::S:					value = Key::S; break;
		case DXKey::T:					value = Key::T; break;
		case DXKey::U:					value = Key::U; break;
		case DXKey::V:					value = Key::V; break;
		case DXKey::W:					value = Key::W; break;
		case DXKey::X:					value = Key::X; break;
		case DXKey::Y:					value = Key::Y; break;
		case DXKey::Z:					value = Key::Z; break;
		case DXKey::KP_0:				value = Key::KP_0; break;
		case DXKey::KP_1:				value = Key::KP_1; break;
		case DXKey::KP_2:				value = Key::KP_2; break;
		case DXKey::KP_3:				value = Key::KP_3; break;
		case DXKey::KP_4:				value = Key::KP_4; break;
		case DXKey::KP_5:				value = Key::KP_5; break;
		case DXKey::KP_6:				value = Key::KP_6; break;
		case DXKey::KP_7:				value = Key::KP_7; break;
		case DXKey::KP_8:				value = Key::KP_8; break;
		case DXKey::KP_9:				value = Key::KP_9; break;
		case DXKey::KP_MULTIPLY:		value = Key::KP_MULTIPLY; break;
		case DXKey::KP_ADD:				value = Key::KP_ADD; break;
		case DXKey::KP_SUBTRACT:		value = Key::KP_SUBTRACT; break;
		case DXKey::KP_DECIMAL:			value = Key::KP_DECIMAL; break;
		case DXKey::KP_DIVIDE:			value = Key::KP_DIVIDE; break;
		case DXKey::F1:					value = Key::F1; break;
		case DXKey::F2:					value = Key::F2; break;
		case DXKey::F3:					value = Key::F3; break;
		case DXKey::F4:					value = Key::F4; break;
		case DXKey::F5:					value = Key::F5; break;
		case DXKey::F6:					value = Key::F6; break;
		case DXKey::F7:					value = Key::F7; break;
		case DXKey::F8:					value = Key::F8; break;
		case DXKey::F9:					value = Key::F9; break;
		case DXKey::F10:				value = Key::F10; break;
		case DXKey::F11:				value = Key::F11; break;
		case DXKey::F12:				value = Key::F12; break;
		case DXKey::F13:				value = Key::F13; break;
		case DXKey::F14:				value = Key::F14; break;
		case DXKey::F15:				value = Key::F15; break;
		case DXKey::F16:				value = Key::F16; break;
		case DXKey::F17:				value = Key::F17; break;
		case DXKey::F18:				value = Key::F18; break;
		case DXKey::F19:				value = Key::F19; break;
		case DXKey::F20:				value = Key::F20; break;
		case DXKey::F21:				value = Key::F21; break;
		case DXKey::F22:				value = Key::F22; break;
		case DXKey::F23:				value = Key::F23; break;
		case DXKey::F24:				value = Key::F24; break;
		case DXKey::NUM_LOCK:			value = Key::NUM_LOCK; break;
		case DXKey::SCROLL_LOCK:		value = Key::SCROLL_LOCK; break;
		case DXKey::LEFT_SHIFT:			value = Key::LEFT_SHIFT; break;
		case DXKey::RIGHT_SHIFT:		value = Key::RIGHT_SHIFT; break;
		case DXKey::LEFT_CONTROL:		value = Key::LEFT_CONTROL; break;
		case DXKey::RIGHT_CONTROL:		value = Key::RIGHT_CONTROL; break;
		case DXKey::LEFT_ALT:			value = Key::LEFT_ALT; break;
		case DXKey::RIGHT_ALT:			value = Key::RIGHT_ALT; break;
		case DXKey::COMMA:				value = Key::COMMA; break;
		case DXKey::MINUS:				value = Key::MINUS; break;
		case DXKey::PERIOD:				value = Key::PERIOD; break;
		default:
			break;
		}
		m_keys[(DXKey)i] = value;
	}
}

ym::Key ym::KeyConverter::convertToKey(DXKey key)
{
	return m_keys[key];
}

ym::MB ym::MBConverter::convertToMB(DXMB mb)
{
	switch (mb)
	{
	case DXMB::LEFT: return MB::LEFT;
	case DXMB::RIGHT: return MB::RIGHT;
	case DXMB::MIDDLE: return MB::MIDDLE;
	default:
		{
			YM_LOG_WARN("Unrecognized mb, cannot convert!");
			return MB::LEFT;
		}
		break;
	}
}

#endif

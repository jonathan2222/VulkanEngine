#pragma once

namespace ym
{
	class System
	{
		// Can filter which entities will be processed by:
		// * All — the archetype must contain all of the component types in the All category.
		// * Any — the archetype must contain at least one of the component types in the Any category.
		// * None — the archetype must not contain any of the component types in the None category.
	};
}
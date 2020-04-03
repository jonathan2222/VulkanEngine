#pragma message("Compiling precompiled headers.\n")

#include <unordered_map>
#include <vector>
#include <string>
#include <optional>

#ifdef YM_PLATFORM_WINDOWS
	#include <Windows.h>
#endif

#include "Engine/Defines.h"
#include "Engine/Core/Logger.h"
#include "Engine/Core/Input/Config.h"

#include "Utils/InstrumentationTimer.h"

#include "vulkan/vulkan.h"

#include "glm/glm.hpp"
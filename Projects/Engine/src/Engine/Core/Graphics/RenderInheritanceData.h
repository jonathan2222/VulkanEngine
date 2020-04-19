#pragma once

#include "Engine/Core/Graphics/ThreadData.h"
#include "Engine/Core/Graphics/SceneData.h"

namespace ym
{
	struct RenderInheritanceData
	{
		std::vector<ThreadData> threadData;
		SceneDescriptors sceneDescriptors;
	};
}
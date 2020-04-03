#pragma once

#include <memory>
#include <cassert>
#include "Core/Logger.h"

#ifdef YM_DEBUG
	#define YM_ASSERT(exp, ...) {if(!(exp)){YM_LOG_CRITICAL(__VA_ARGS__);} assert(exp); }
#else
	#define YM_ASSERT(exp, ...) {assert(exp);}
#endif

#define VULKAN_CHECK(res, msg) if (res != VK_SUCCESS) { YM_LOG_ERROR(msg); throw std::runtime_error(msg); }

#define Ptr(T) std::unique_ptr<T>
#define Ref(T) std::shared_ptr<T>

#define MakePtr(T, ...) std::make_unique<T>(__VA_ARGS__)
#define MakeRef(T, ...) std::make_shared<T>(__VA_ARGS__)

#define YM_CONFIG_FILE_PATH "./Resources/Config/EngineConfig.json"
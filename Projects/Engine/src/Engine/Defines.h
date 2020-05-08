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
#define FMOD_CHECK(res, ...) { FMOD_RESULT r = res; if(r != FMOD_OK && r != FMOD_ERR_INVALID_HANDLE) { YM_LOG_ERROR("FMOD error! ({0}) {1}", r, FMOD_ErrorString(res)); YM_ASSERT(false, __VA_ARGS__); }}

#define Ptr(T) std::unique_ptr<T>
#define Ref(T) std::shared_ptr<T>

#define MakePtr(T, ...) std::make_unique<T>(__VA_ARGS__)
#define MakeRef(T, ...) std::make_shared<T>(__VA_ARGS__)

#define SAFE_DELETE(p) if(p){delete p; p = nullptr;}
#define SAFE_DELETE_ARR(arr) if(arr){delete[] arr; arr = nullptr;}
#define SIZE_OF_ARR(arr) (sizeof(arr)/sizeof(arr[0]))

#define YM_ASSETS_FILE_PATH std::string("./Resources/")
#define YM_CONFIG_FILE_PATH "./Resources/Config/EngineConfig.json"
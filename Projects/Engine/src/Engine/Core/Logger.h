#pragma once

#pragma warning( push )
#pragma warning( disable : 26451 )
#pragma warning( disable : 26439 )
#pragma warning( disable : 26495 )
#include "spdlog/spdlog.h"
#pragma warning( pop )

#ifdef YAMI_DEBUG
	#define YM_LOG_INIT() \
		spdlog::set_level(spdlog::level::info); \
		spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] [%s:%#] %v")

	#define YM_LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)
	#define YM_LOG_WARN(...) SPDLOG_WARN(__VA_ARGS__)
	#define YM_LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
	#define YM_LOG_CRITICAL(...) SPDLOG_CRITICAL(__VA_ARGS__)
	#define YM_LOG_SUCCESS(...) SPDLOG_INFO(__VA_ARGS__)
#else
	#define YM_LOG_INIT()
	// TODO: Should this be empty?
	#define YM_LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)
	#define YM_LOG_WARN(...)
	#define YM_LOG_ERROR(...)
	#define YM_LOG_CRITICAL(...)
	#define YM_LOG_SUCCESS(...)
#endif

workspace "VulkanEngine"
	architecture "x86_64"
	startproject "Game"

	configurations { "Debug", "Release" }

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"

		defines { "YM_PLATFORM_WINDOWS" }

	filter "configurations:Debug"
		defines "YM_DEBUG"
		symbols "On"

	filter "configurations:Release"
		defines "YM_RELEASE"
		optimize "On"
	filter {}

OUTPUT_DIR = "%{cfg.buildcfg}_%{cfg.system}_%{cfg.architecture}"

-- ========================================== GLFW ==========================================

function includeGLFW()
	includedirs "Externals/GLFW/Include/"
end

function linkGLFW()
	libdirs "Externals/GLFW/Lib/%{cfg.buildcfg}"

	filter "system:windows"
		links { "opengl32" }

	filter "system:not windows"
		links { "GL" }

	-- Only the StaticLibrary should link against GLFW.
	filter "kind:not StaticLib"
		links "glfw3"
	filter {} -- Reset the filters for other settings.
end

-- ==============================================================================================

-- ============================================ MISC ============================================

function setTargetAndObjDirs()
	targetdir ("Build/Bin/" .. OUTPUT_DIR .. "/%{prj.name}")
	objdir ("Build/Obj/" .. OUTPUT_DIR .. "/%{prj.name}")
end

function addFiles()
	files
	{
		"Projects/%{prj.name}/src/**.h",
		"Projects/%{prj.name}/src/**.hpp",
		"Projects/%{prj.name}/src/**.cpp"
	}
end

-- ==============================================================================================

-- =========================================== SPDLOG ===========================================

function includeSpdlog()
	includedirs { "Externals/SPDLOG/Include" }
end

function useSpdlog()
	includeSpdlog()

	filter "configurations:Debug"
		libdirs "Externals/SPDLOG/Lib/Debug/spdlogd.lib"

	filter "configurations:Release"
		libdirs "Externals/SPDLOG/Lib/Release/spdlog.lib"
	
	filter {}
end

-- ==============================================================================================

-- =========================================== SPDLOG ===========================================

function includeGLM()
	includedirs { "Externals/GLM/" }
end

-- ==============================================================================================

-- ==============================================================================================

function useEngine()
	includedirs { "Projects/Engine/src" }
	links "Engine"

	includeGLM()
	includeGLFW()
	linkGLFW()
	includeSpdlog()
end

-- ========================================== PROJECTS ==========================================

project "Game"
	location "Projects/Game"
	kind "ConsoleApp"
	language "C++"

	setTargetAndObjDirs()

	addFiles();

	useEngine()
	
	filter {}

project "Engine"
	location "Projects/Engine"
	kind "StaticLib"
	language "C++"
	defines {"GLEW_STATIC"}

	setTargetAndObjDirs()

	pchheader "stdafx.h"
	pchsource "Projects/%{prj.name}/src/stdafx.cpp"
	includedirs { "Projects/%{prj.name}/src/" }

	addFiles();

	includeGLFW()
	includeGLM()

	useSpdlog()

	

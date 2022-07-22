workspace "Usul"
	architecture "x64"
	startproject "Sandbox"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["GLFW"] = "Usul/vendor/GLFW/include"
IncludeDir["Glad"] = "Usul/vendor/Glad/include"
IncludeDir["ImGui"] = "Usul/vendor/imgui"
IncludeDir["glm"] = "Usul/vendor/glm"

group "Dependencies"
	include "Usul/vendor/GLFW"
	include "Usul/vendor/Glad"
	include "Usul/vendor/imgui"

group ""

project "Usul"
	location "Usul"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-intermediate/" .. outputdir .. "/%{prj.name}")

	pchheader "uspch.h"
	pchsource "Usul/src/uspch.cpp"

	defines
	{
		"_CRT_SECURE_NO_WARNINGS"
	}

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/vendor/glm/glm/**.hpp",
		"%{prj.name}/vendor/glm/glm/**.inl",
	}

	includedirs 
	{
		"%{prj.name}/vendor/spdlog/include",
		"%{prj.name}/src",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.glm}"
	}

	links 
	{ 
		"GLFW",
		"Glad",
		"ImGui",
		"opengl32.lib"
	}

	filter "system:windows"
		
		systemversion "latest"

		defines
		{
			"US_PLATFORM_WINDOWS",
			"US_BUILD_DLL",
			"GLFW_INCLUDE_NONE"
		}

	filter "configurations:Debug"
		defines "US_DEBUG"
		symbols "on"
		runtime "Debug"
	
	filter "configurations:Release"
	defines "US_RELEASE"
	runtime "Release"
	optimize "on"

	filter "configurations:Dist"
	defines "US_DIST"
	runtime "Release"
	optimize "on"


project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-intermediate/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs 
	{
		"Usul/vendor/spdlog/include",
		"Usul/src",
		"Usul/vendor",
		"%{IncludeDir.glm}"
	}

	links
	{
		"Usul"
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"US_PLATFORM_WINDOWS"
		}

	filter "configurations:Debug"
		defines "US_DEBUG"
		runtime "Debug"
		symbols "on"
	
	filter "configurations:Release"
	defines "US_RELEASE"
	runtime "Release"
	optimize "on"

	filter "configurations:Dist"
	defines "US_DIST"
	runtime "Release"
	optimize "on"
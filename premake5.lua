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
IncludeDir["ImGui"] = "Usul/vendor/imgui/include"
IncludeDir["glm"] = "Usul/vendor/glm"

group "Dependencies"
	include "Usul/vendor/GLFW"
	include "Usul/vendor/Glad"
	include "Usul/vendor/imgui"

group ""

project "Usul"
	location "Usul"
	kind "SharedLib"
	language "C++"
	staticruntime "off"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-intermediate/" .. outputdir .. "/%{prj.name}")

	pchheader "uspch.h"
	pchsource "Usul/src/uspch.cpp"

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
		cppdialect "C++17"
		systemversion "latest"

		defines
		{
			"US_PLATFORM_WINDOWS",
			"US_BUILD_DLL",
			"GLFW_INCLUDE_NONE"
		}

		postbuildcommands
		{
			--("{COPY} ../bin/" .. outputdir.. "/Usul/*.dll ../bin/"..outputdir.."/Sandbox")
			("{COPY} %{cfg.buildtarget.relpath} \"../bin/" .. outputdir .. "/Sandbox/\"")
			--("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/Sandbox")
		}

	filter "configurations:Debug"
		defines "US_DEBUG"
		symbols "On"
		runtime "Debug"
	
	filter "configurations:Release"
	defines "US_RELEASE"
	runtime "Release"
	optimize "On"

	filter "configurations:Dist"
	defines "US_DIST"
	runtime "Release"
	optimize "On"


project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"

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
		"%{IncludeDir.glm}"
	}

	links
	{
		"Usul"
	}

	filter "system:windows"
		cppdialect "C++17"
		systemversion "latest"

		defines
		{
			"US_PLATFORM_WINDOWS"
		}

	filter "configurations:Debug"
		defines "US_DEBUG"
		runtime "Debug"
		symbols "On"
	
	filter "configurations:Release"
	defines "US_RELEASE"
	runtime "Release"
	optimize "On"

	filter "configurations:Dist"
	defines "US_DIST"
	runtime "Release"
	optimize "On"
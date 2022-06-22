workspace "Usul"
	architecture "x64"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "Usul"
	location "Usul"
	kind "SharedLib"
	language "C++"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-intermediate/" .. outputdir .. "/%{prj.name}")

	pchheader "uspch.h"
	pchsource "Usul/src/uspch.cpp"

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs 
	{
		"%{prj.name}/vendor/spdlog/include",
		"%{prj.name}/src"
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"

		defines
		{
			"US_PLATFORM_WINDOWS",
			"US_BUILD_DLL"
		}

	filter "configurations:Debug"
		defines "US_DEBUG"
		symbols "On"
	
	filter "configurations:Release"
	defines "US_RELEASE"
	optimize "On"

	filter "configurations:Dist"
	defines "US_DIST"
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
		"Usul/src"
	}

	links
	{
		"Usul"
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"

		defines
		{
			"US_PLATFORM_WINDOWS"
		}

		postbuildcommands
		{
			{"{COPY} ../bin/" .. outputdir.. "/Usul/*.dll ../bin/"..outputdir.."/Sandbox"}
		}

	filter "configurations:Debug"
		defines "US_DEBUG"
		symbols "On"
	
	filter "configurations:Release"
	defines "US_RELEASE"
	optimize "On"

	filter "configurations:Dist"
	defines "US_DIST"
	optimize "On"
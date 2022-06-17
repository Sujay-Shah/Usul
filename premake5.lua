workspace "Usul"
	architecture "x64"

	configuration
	{
		"Debug",
		"Release",
		"Dist"
	}

outputDir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "Usul"
	location "Usul"
	kind "SharedLib"
	language "C++"

	targetDir ("bin/" .. outputdir .. "/%{prj.name}")
	objDir ("bin-intermediate/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs 
	{
		"%{prj.name}\vendor\spdlog\include"
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

		postbuild
		{
			{"{COPY} &{cfg.buildtarget.relpath} ../bin/" .. outputDir.. "/Sandbox"}
		}

	filter "configuration:Debug"
		defines "US_DEBUG"
		symbols "On"
	
	filter "configuration:Release"
	defines "US_RELEASE"
	optimize "On"

	filter "configuration:Dist"
	defines "US_DIST"
	optimize "On"
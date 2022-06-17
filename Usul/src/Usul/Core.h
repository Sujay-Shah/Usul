#pragma once

#ifdef US_PLATFORM_WINDOWS
	#ifdef US_BUILD_DLL
		#define USUL_API __declspec(dllexport)
	#else
		#define USUL_API __declspec(dllimport)
	#endif
#else
	#error Usul support only windows for now!
#endif
#pragma once

#ifdef US_PLATFORM_WINDOWS
	#if US_DYNAMIC_LINK
		#ifdef US_BUILD_DLL
			#define USUL_API __declspec(dllexport)
		#else
			#define USUL_API __declspec(dllimport)
		#endif
	#else
	#define USUL_API
	#endif
#else
	#error Usul support only windows for now!
#endif

#define BIT(x) (1 << x)

#ifdef US_ENABLE_ASSERTS
#define US_ASSERT(x, ...) { if(!(x)) { US_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#define US_CORE_ASSERT(x, ...) { if(!(x)) { US_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
#define US_ASSERT(x, ...)
#define US_CORE_ASSERT(x, ...)
#endif

#define BIND_EVENT_CALLBK_FN(x) std::bind(&x,this,std::placeholders::_1)
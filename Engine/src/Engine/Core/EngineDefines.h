#ifndef __DEFINES_H__
#define __DEFINES_H__

#include "Engine/Core/PlatformDetection.h"

#include <memory>

#ifdef ENGINE_DEBUG
	#if defined(ENGINE_PLATFORM_WINDOWS)
	#define ENGINE_DEBUGBREAK() __debugbreak()
	#elif defined(ENGINE_PLATFORM_LINUX) || defined(ENGINE_PLATFORM_MAC)
	#include <signal.h>
	#define ENGINE_DEBUGBREAK() raise(SIGTRAP)
	#else
	#error "Platform doesn't support debugbreak yet!"
	#endif
	#define ENGINE_ENABLE_ASSERTS
#else
#define ENGINE_DEBUGBREAK()
#endif

#define ENGINE_EXPAND_MACRO(x) x
#define ENGINE_STRINGIFY_MACRO(x) #x

#define BIT(x) (1 << x)

#define ENGINE_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

namespace Engine {

	template<typename T>
	using Scope = std::unique_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Scope<T> CreateScope(Args&& ... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	using Ref = std::shared_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Ref<T> CreateRef(Args&& ... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

}

#include "Engine/Core/Logging.h"
#include "Engine/Core/Assert.h"

#endif
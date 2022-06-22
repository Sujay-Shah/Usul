#pragma once

#include "Core.h"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"
#include <memory>

namespace Usul
{
	class USUL_API Log
	{
	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};
}

//core log macros
#define US_CORE_TRACE(...)	::Usul::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define US_CORE_INFO(...)	::Usul::Log::GetCoreLogger()->info(__VA_ARGS__)
#define US_CORE_WARN(...)	::Usul::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define US_CORE_ERROR(...)	::Usul::Log::GetCoreLogger()->error(__VA_ARGS__)
#define US_CORE_FATAL(...)	::Usul::Log::GetCoreLogger()->fatal(__VA_ARGS__)

//client log macros
#define US_TRACE(...)	::Usul::Log::GetClientLogger()->trace(__VA_ARGS__)
#define US_INFO(...)	::Usul::Log::GetClientLogger()->info(__VA_ARGS__)
#define US_WARN(...)	::Usul::Log::GetClientLogger()->warn(__VA_ARGS__)
#define US_ERROR(...)	::Usul::Log::GetClientLogger()->error(__VA_ARGS__)
#define US_FATAL(...)	::Usul::Log::GetClientLogger()->fatal(__VA_ARGS__)



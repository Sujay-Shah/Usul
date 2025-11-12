#ifndef __LOGGING_H__
#define __LOGGING_H__

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"

// This ignores all warnings raised inside External headers
#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)

namespace Engine
{
    class Log
    {
        public:
            Log();
            ~Log();
            
            static void Init();

            inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return m_engineLogger; }
            inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return m_clientLogger; }
        private:
            static std::shared_ptr<spdlog::logger> m_engineLogger;
            static std::shared_ptr<spdlog::logger> m_clientLogger;
    };
}

template<typename OStream, glm::length_t L, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::vec<L, T, Q>& vector)
{
	return os << glm::to_string(vector);
}

template<typename OStream, glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::mat<C, R, T, Q>& matrix)
{
	return os << glm::to_string(matrix);
}

template<typename OStream, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, glm::qua<T, Q> quaternion)
{
	return os << glm::to_string(quaternion);
}

#define ENGINE_CORE_TRACE(...) Engine::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define ENGINE_CORE_INFO(...) Engine::Log::GetCoreLogger()->info(__VA_ARGS__)
#define ENGINE_CORE_WARN(...) Engine::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define ENGINE_CORE_ERROR(...) Engine::Log::GetCoreLogger()->error(__VA_ARGS__)
#define ENGINE_CORE_DEBUG(...) Engine::Log::GetCoreLogger()->debug(__VA_ARGS__)

#define ENGINE_TRACE(...) Engine::Log::GetClientLogger()->trace(__VA_ARGS__)
#define ENGINE_INFO(...) Engine::Log::GetClientLogger()->info(__VA_ARGS__)
#define ENGINE_WARN(...) Engine::Log::GetClientLogger()->warn(__VA_ARGS__)
#define ENGINE_ERROR(...) Engine::Log::GetClientLogger()->error(__VA_ARGS__)
#define ENGINE_DEBUG(...) Engine::Log::GetClientLogger()->debug(__VA_ARGS__)

#endif

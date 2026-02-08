#include "RendererAPI.h"

namespace Engine
{
#if API_OPENGL
    RendererAPI::API RendererAPI::m_api = RendererAPI::API::OpenGL;
#elif API_VULKAN
    RendererAPI::API RendererAPI::m_api = RendererAPI::API::Vulkan;
#endif
}
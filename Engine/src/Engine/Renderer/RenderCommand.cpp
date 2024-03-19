#include "RenderCommand.h"
#include "Platform/OpenGL/RendererAPIOpenGL.h"
#include "Platform/Vulkan/RendererAPIVulkan.h"

namespace Engine
{
#if API_VULKAN
    RendererAPI* RenderCommand::m_rendererAPI = new RendererAPIVulkan;
#else
    RendererAPI* RenderCommand::m_rendererAPI = new RendererAPIOpenGL;
#endif
}
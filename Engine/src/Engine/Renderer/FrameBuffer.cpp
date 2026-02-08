#include "../EnginePCH.h"
#include "FrameBuffer.h"
#include "RHI/Renderer.h"
#include "RHI/Backends/OpenGL/FrameBufferOpenGL.h"

namespace Engine {

    Ref<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
    {
        switch (RendererAPI::GetAPI())
        {
            case RendererAPI::API::None:    ENGINE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
            case RendererAPI::API::OpenGL:  return CreateRef<FrameBufferOpenGL>(spec);
        }

        ENGINE_CORE_ASSERT(false, "Unknown RendererAPI!");
        return nullptr;
    }

} // Engine
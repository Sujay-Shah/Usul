#include "FrameBuffer.h"
#include "Renderer.h"
#include "Backends/OpenGL/FrameBufferOpenGL.h"

namespace Engine
{
    Ref<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:    return nullptr;
            case RendererAPI::API::OpenGL:  return std::make_shared<FrameBufferOpenGL>(spec);
        }

        return nullptr;
    }
}

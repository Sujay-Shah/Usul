#include "VertexArray.h"
#include "RHI/Renderer.h"
#include "RHI/Backends/OpenGL/VertexArrayOpenGL.h"

namespace Engine
{
    Ref<VertexArray> VertexArray::Create()
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:    return nullptr;
            case RendererAPI::API::OpenGL:  return std::make_shared<VertexArrayOpenGL>();
        }

        return nullptr;
    }
}

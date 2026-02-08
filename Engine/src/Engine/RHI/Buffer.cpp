#include "Buffer.h"
#include "Renderer.h"
#include "Backends/OpenGL/BufferOpenGL.h"

namespace Engine
{
    Ref<VertexBuffer> VertexBuffer::Create(uint32_t size)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:    return nullptr;
            case RendererAPI::API::OpenGL:  return std::make_shared<VertexBufferOpenGL>(size);
        }

        return nullptr;
    }

    Ref<VertexBuffer> VertexBuffer::Create(void* vertices, uint32_t size)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:    return nullptr;
            case RendererAPI::API::OpenGL:  return std::make_shared<VertexBufferOpenGL>(vertices, size);
        }

        return nullptr;
    }

    Ref<IndexBuffer> IndexBuffer::Create(uint32_t* indices, uint32_t count)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:    return nullptr;
            case RendererAPI::API::OpenGL:  return std::make_shared<IndexBufferOpenGL>(indices, count);
        }

        return nullptr;
    }
}

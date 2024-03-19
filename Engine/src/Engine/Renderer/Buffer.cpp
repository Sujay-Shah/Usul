#include "Buffer.h"
#include "Renderer.h"
#include "Platform/OpenGL/BufferOpenGL.h"

#include "Engine/Core/Logging.h"
#include "Engine/Core/EngineDefines.h"

namespace Engine
{
    Ref<VertexBuffer> VertexBuffer::Create(float* vertices, uint32_t size)
    {
        switch(Renderer::Get())
        {
            case RendererAPI::API::None:
                ENGINE_ASSERT(0, "No renderer API is not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return std::make_shared<VertexBufferOpenGL>(vertices, size);
            default:
                ENGINE_ASSERT(0, "Unsupported/unknown render API!");
                return nullptr;
        }
    }

    Ref <VertexBuffer> VertexBuffer::Create(Vertex *vertices, uint32_t size) {
        switch(Renderer::Get())
        {
            case RendererAPI::API::None:
            ENGINE_ASSERT(0, "No renderer API is not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                // A great thing about structs is that their memory layout is sequential for all its items.
                // The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
                // again translates to 3/2 floats which translates to a byte array.
                return std::make_shared<VertexBufferOpenGL>(vertices, size);
            default:
            ENGINE_ASSERT(0, "Unsupported/unknown render API!");
                return nullptr;
        }
    }

    Ref<IndexBuffer> IndexBuffer::Create(uint32_t* indices, uint32_t count)
    {
        switch(Renderer::Get())
        {
            case RendererAPI::API::None:
                ENGINE_ASSERT(0, "No renderer API is not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return std::make_shared<IndexBufferOpenGL>(indices, count);
            default:
                ENGINE_ASSERT(0, "Unsupported/unknown render API!");
                return nullptr;
        }
    }
}
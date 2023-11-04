#include "VertexArray.h"
#include "Renderer.h"
#include "Platform/OpenGL/VertexArrayOpenGL.h"

#include "Engine/Core/Logging.h"
#include "Engine/Core/EngineDefines.h"

namespace Engine
{
    Ref<VertexArray> VertexArray::Create()
    {
        switch(Renderer::Get())
        {
            case RendererAPI::API::None:
                ENGINE_ASSERT(0, "No render API is not suppoted!")
                return nullptr;
            case RendererAPI::API::OpenGL:
                return std::make_shared<VertexArrayOpenGL>();
            default:
                ENGINE_ASSERT(0, "Unknown render API!");
                return nullptr;
        }
    }
}

#include "Engine/Core/Logging.h"
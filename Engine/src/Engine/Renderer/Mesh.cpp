#include "Renderer/Mesh.h"
#include "RHI/Backends/OpenGL/MeshOpenGL.h"

namespace Engine
{
    Ref<Mesh> Mesh::CreateMesh(std::vector<Vertex> vertices, std::vector<uint32> indices, std::vector<Ref<Texture2D>> textures)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:    return nullptr;
            case RendererAPI::API::OpenGL:  return std::make_shared<MeshOpenGL>(vertices, indices, textures);
        }

        return nullptr;
    }
}

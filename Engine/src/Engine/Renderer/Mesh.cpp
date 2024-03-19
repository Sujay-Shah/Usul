//
// Created by snsha on 2024-03-15.
//
#include "Platform/OpenGL/MeshOpenGl.h"
#include "Engine/Core/EngineDefines.h"

namespace Engine
{
    Mesh::Mesh(std::vector<Vertex>& vertices, std::vector<uint32>& indices, std::vector<Ref<Texture2D>>& textures)
    {
        m_vertices = vertices;
        m_indices = indices;
        m_textures = textures;
    }
    Ref<Mesh> Mesh::CreateMesh(std::vector<Vertex> vertices, std::vector<uint32> indices, std::vector<Ref<Texture2D>> textures)
    {
        switch(Renderer::Get())
        {
        case RendererAPI::API::None:
        ENGINE_ASSERT(0, "No render API is not suppoted!")
        return nullptr;
        case RendererAPI::API::OpenGL:
        return std::make_shared<MeshOpenGL>(vertices,indices,textures);
        default:
        ENGINE_ASSERT(0, "Unknown render API!");
        return nullptr;
        }
    }
}
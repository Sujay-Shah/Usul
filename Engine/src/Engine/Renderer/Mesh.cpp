//
// Created by snsha on 2024-03-15.
//
#include "Mesh.h"
#include "Engine/Core/EngineDefines.h"

namespace Engine
{
    Mesh::Mesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, std::vector<rhi::Texture>& textures)
    {
        m_vertices = vertices;
        m_indices = indices;
        m_textures = textures;
    }
    
    Ref<Mesh> Mesh::CreateMesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices, std::vector<rhi::Texture> textures)
    {
        return CreateRef<Mesh>(vertices, indices, textures);
    }
}
//
// Created by snsha on 2024-01-11.
//

#ifndef USUL_MESH_H
#define USUL_MESH_H

#include "RHI/rhi_types.hpp"
#include "Renderer/Vertex.h"
#include "Engine/Core/EngineDefines.h"

namespace Engine
{
    class Shader;
    class Mesh
    {
    public:

        Mesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, std::vector<rhi::Texture>& textures);

        virtual void Draw(Ref<Shader> shader){};

        virtual ~Mesh(){};

        static Ref<Mesh> CreateMesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices, std::vector<rhi::Texture> textures);

        std::vector<Vertex> m_vertices;
        std::vector<uint32_t> m_indices;
        std::vector<rhi::Texture> m_textures;

    protected:
        rhi::Buffer m_vbo;
        rhi::Buffer m_ibo;

        virtual void SetupMesh(){};
    };

} // Engine

#endif //USUL_MESH_H

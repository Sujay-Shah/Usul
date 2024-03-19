//
// Created by snsha on 2024-01-11.
//

#ifndef USUL_MESH_H
#define USUL_MESH_H

#include "Renderer/Texture.h"
#include "Renderer/Vertex.h"
#include "Buffer.h"
#include "Renderer.h"

namespace Engine
{
    class Mesh
    {
    public:

        Mesh(std::vector<Vertex>& vertices, std::vector<uint32>& indices, std::vector<Ref<Texture2D>>& textures);

        virtual void Draw(Ref<Shader> shader){};

        virtual ~Mesh(){};

        static Ref<Mesh> CreateMesh(std::vector<Vertex> vertices, std::vector<uint32> indices, std::vector<Ref<Texture2D>> textures);

        std::vector<Vertex> m_vertices;
        std::vector<uint32_t> m_indices;
        std::vector<Ref<Texture2D>> m_textures;

    protected:
        Ref<VertexArray> m_vao;
        Ref<VertexBuffer> m_vbo;
        Ref<IndexBuffer> m_ibo;

        virtual void SetupMesh(){};
    };




} // Engine

#endif //USUL_MESH_H

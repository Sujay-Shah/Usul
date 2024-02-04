//
// Created by snsha on 2024-01-11.
//

#ifndef USUL_MESH_H
#define USUL_MESH_H

#include "Renderer/Texture.h"
#include "Renderer/Vertex.h"

namespace Engine
{
    class Shader;
    class Material;

    class Mesh
    {
    public:

        virtual void Draw(Shader& shader){};

        virtual ~Mesh(){};

        std::vector<Vertex> m_vertices;
        std::vector<unsigned int> m_indices;

    private:
        Ref<Material> m_material;
    };

} // Engine

#endif //USUL_MESH_H

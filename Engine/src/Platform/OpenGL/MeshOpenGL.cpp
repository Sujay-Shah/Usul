//
// Created by snsha on 2024-01-12.
//

#include "MeshOpenGL.h"


namespace Engine {
    void MeshOpenGL::Draw(Shader &shader)
    {
        Mesh::Draw(shader);
    }

    void MeshOpenGL::setupMesh()
    {

    }

    MeshOpenGL::MeshOpenGL(std::vector<Vertex> vertices, std::vector<uint32> indices, std::vector<Texture2DOpenGL> textures)
    {
        m_vertices = vertices;
        m_indices = indices;
        m_textures = textures;
    }

    MeshOpenGL::~MeshOpenGL() {

    }
} // Engine
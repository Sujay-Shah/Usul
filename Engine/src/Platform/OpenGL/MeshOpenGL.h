//
// Created by snsha on 2024-01-12.
//

#ifndef USUL_MESHOPENGL_H
#define USUL_MESHOPENGL_H
#include "Renderer/Mesh.h"
#include "TextureOpenGL.h"

namespace Engine {

    class MeshOpenGL : public Mesh
    {
    public:
        MeshOpenGL(std::vector<Vertex> vertices, std::vector<uint32> indices, std::vector<Texture2DOpenGL> textures);
        ~MeshOpenGL();

        void Draw(Shader &shader) override;

        std::vector<Texture2DOpenGL> m_textures;
    private:

        uint32 m_VAO, m_VBO, m_EBO;

        void setupMesh();
    };

} // Engine

#endif //USUL_MESHOPENGL_H

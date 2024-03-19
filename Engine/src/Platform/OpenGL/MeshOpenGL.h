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
        MeshOpenGL(std::vector<Vertex> vertices, std::vector<uint32> indices, std::vector<Ref<Texture2D>> textures);
        ~MeshOpenGL();

        void Draw(Ref<Shader> shader) override;


    protected:
        void SetupMesh() override;
    };

} // Engine

#endif //USUL_MESHOPENGL_H

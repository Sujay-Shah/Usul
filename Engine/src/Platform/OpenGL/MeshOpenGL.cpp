//
// Created by snsha on 2024-01-12.
//

#include "MeshOpenGL.h"
#include "ShaderOpenGL.h"


namespace Engine {
    void MeshOpenGL::Draw(Ref<Shader> currShader)
    {
        // bind appropriate textures
        uint32_t diffuseNr  = 1;
        uint32_t specularNr = 1;
        uint32_t normalNr   = 1;
        uint32_t heightNr   = 1;

        //
        Ref<ShaderOpenGL> shader = std::dynamic_pointer_cast<ShaderOpenGL>(currShader);

        if(!shader)
        {
            ENGINE_CORE_WARN("Shader pointer null at {0} {1}",__FILE__,__LINE__);
        }

        //TODO: refactor this to do it some other way
        for(unsigned int i = 0; i < m_textures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i); // active proper texture unit before binding
            // retrieve texture number (the N in diffuse_textureN)
            std::string number;
            std::string name;
            TextureType::Type type = m_textures[i]->GetTextureType();
            if (type == TextureType::Type::Diffuse) {
                number = std::to_string(diffuseNr++);
                name = "texture_diffuse";
            } else if (type == TextureType::Type::Specular) {
                number = std::to_string(specularNr++);
                name = "texture_specular";
            } else if (type == TextureType::Type::NormalMap)
            {
                number = std::to_string(normalNr++);
                name = "texture_normal";
            }
            else if(type == TextureType::Type::HeightMap)
            {
                number = std::to_string(heightNr++);
                name = "texture_height";
            }


            // now set the sampler to the correct texture unit
            glUniform1i(glGetUniformLocation(shader->GetRendererId(), (name + number).c_str()), i);
            // and finally bind the texture
            glBindTexture(GL_TEXTURE_2D, m_textures[i]->GetRendererID());
        }

        // draw mesh
        Renderer::Submit(shader,m_vao);
        //m_vao->Bind();
        //glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(m_indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // always good practice to set everything back to defaults once configured.
        glActiveTexture(GL_TEXTURE0);


    }

    MeshOpenGL::MeshOpenGL(std::vector<Vertex> vertices, std::vector<uint32> indices, std::vector<Ref<Texture2D>> textures):
            Mesh(vertices,indices,textures)
    {
        // now that we have all the required data, set the vertex buffers
        // and its attribute pointers.
        this->SetupMesh();
    }

    MeshOpenGL::~MeshOpenGL()
    {

    }

    void MeshOpenGL::SetupMesh()
    {
        //create buffers,arrays
        m_vao = VertexArray::Create();
        m_vbo = VertexBuffer::Create(m_vertices.data(), m_vertices.size());


        m_vbo->SetLayout({
            {Engine::ShaderDataType::Float3, "aPos"},
            {Engine::ShaderDataType::Float3, "aNormal"},
            {Engine::ShaderDataType::Float2, "aTexCoords"},
            {Engine::ShaderDataType::Float3, "aTangent"},
            {Engine::ShaderDataType::Float3, "aBiTangent"},
            {Engine::ShaderDataType::Float4, "aBoneId"},
            {Engine::ShaderDataType::Float4, "aBoneWeight"},
        });

        m_vao->AddVertexBuffer(m_vbo);

        m_ibo = IndexBuffer::Create(m_indices.data(),m_indices.size());
        m_vao->SetIndexBuffer(m_ibo);


    }
} // Engine
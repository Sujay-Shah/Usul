//
// Created by snsha on 2024-03-13.
//

#ifndef USUL_MODEL_H
#define USUL_MODEL_H

#include "Mesh.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Core/EngineDefines.h"
#include "Core/AssetManager.h"

namespace Engine
{
    class Model
    {
    public:
        Model(std::string const &path, bool gamma = false) : m_gammaCorrection(gamma)
        {
            loadModel(AssetManager::GetAssetPath(path).string());
        }
        std::vector<Ref<Texture2D>> m_textures_loaded;
        std::vector<Ref<Mesh>> m_meshes;
        std::string m_directory;
        bool m_gammaCorrection;

        void Draw(Ref<Shader> shader)
        {
            for(uint32_t i = 0; i < m_meshes.size(); i++)
                m_meshes[i]->Draw(shader);
        };

    protected:

        // checks all material textures of a given type and loads the textures if they're not loaded yet.
// the required info is returned as a Texture struct.
        std::vector<Ref<Texture2D>> loadMaterialTextures(aiMaterial *mat, aiTextureType type, TextureType::Type typeName)
        {
            std::vector<Ref<Texture2D>> textures;
            for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
            {
                aiString str;
                mat->GetTexture(type, i, &str);
                // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
                bool skip = false;
                for(unsigned int j = 0; j < m_textures_loaded.size(); j++)
                {

                    std::string tex = m_textures_loaded[j]->m_path.substr(m_textures_loaded[j]->m_path.find_last_of('/')+1,m_textures_loaded[j]->m_path.size());
                    if(std::strcmp(tex.c_str(), str.C_Str()) == 0)
                    {
                        textures.push_back(m_textures_loaded[j]);
                        skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
                        break;
                    }
                }
                if(!skip)
                {   // if texture hasn't been loaded already, load it
                    Ref<Texture2D> texture = Texture2D::Create( m_directory + str.C_Str() );
                    texture->m_type = typeName;
                    textures.push_back(texture);
                    m_textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecessary load duplicate textures.
                }
            }
            return textures;
        }

        void loadModel(std::string const & path){
            // read file via ASSIMP
            Assimp::Importer importer;
            const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
            // check for errors
            if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
            {
               ENGINE_CORE_ERROR( "ERROR::ASSIMP:: {0}", importer.GetErrorString());
                return;
            }
            // retrieve the directory path of the filepath
            m_directory = path.substr(0, path.find_last_of('/') + 1);

            // process ASSIMP's root node recursively
            processNode(scene->mRootNode, scene);
        };
        // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
        void processNode(aiNode * node, const aiScene * scene)
        {
            // process each mesh located at the current node
            for(uint32_t i = 0; i < node->mNumMeshes; i++)
            {
                // the node object only contains indices to index the actual objects in the scene.
                // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
                aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
                m_meshes.push_back(processMesh(mesh, scene));
            }
            // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
            for(uint32_t i = 0; i < node->mNumChildren; i++)
            {
                processNode(node->mChildren[i], scene);
            }
        };

        Ref<Mesh> processMesh(aiMesh * mesh,const aiScene * scene)
        {
            // data to fill
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;
            std::vector<Ref<Texture2D>> textures;

// walk through each of the mesh's vertices
            for(uint32_t i = 0; i < mesh->mNumVertices; i++)
            {
                Vertex vertex;
                glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
                // positions
                vector.x = mesh->mVertices[i].x;
                vector.y = mesh->mVertices[i].y;
                vector.z = mesh->mVertices[i].z;
                vertex.Position = vector;
                // normals
                if (mesh->HasNormals())
                {
                    vector.x = mesh->mNormals[i].x;
                    vector.y = mesh->mNormals[i].y;
                    vector.z = mesh->mNormals[i].z;
                    vertex.Normal = vector;
                }
                // texture coordinates
                if(mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
                {
                    glm::vec2 vec;
                    // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
                    // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
                    vec.x = mesh->mTextureCoords[0][i].x;
                    vec.y = mesh->mTextureCoords[0][i].y;
                    vertex.TexCoords = vec;
                    // tangent
                    vector.x = mesh->mTangents[i].x;
                    vector.y = mesh->mTangents[i].y;
                    vector.z = mesh->mTangents[i].z;
                    vertex.Tangent = vector;
                    // bitangent
                    vector.x = mesh->mBitangents[i].x;
                    vector.y = mesh->mBitangents[i].y;
                    vector.z = mesh->mBitangents[i].z;
                    vertex.Bitangent = vector;
                }
                else
                    vertex.TexCoords = glm::vec2(0.0f, 0.0f);

                vertices.push_back(vertex);
            }
// now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
            for(uint32_t i = 0; i < mesh->mNumFaces; i++)
            {
                aiFace face = mesh->mFaces[i];
                // retrieve all indices of the face and store them in the indices vector
                for(uint32_t j = 0; j < face.mNumIndices; j++)
                    indices.push_back(face.mIndices[j]);
            }
// process materials
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
// we assume a convention for sampler names in the shaders. Each diffuse texture should be named
// as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER.
// Same applies to other texture as the following list summarizes:
// diffuse: texture_diffuseN
// specular: texture_specularN
// normal: texture_normalN

// 1. diffuse maps
            std::vector<Ref<Texture2D>> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, TextureType::Type::Diffuse);
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
// 2. specular maps
            std::vector<Ref<Texture2D>> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, TextureType::Type::Specular);
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
// 3. normal maps
            std::vector<Ref<Texture2D>> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, TextureType::Type::HeightMap);
            textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
// 4. height maps
            std::vector<Ref<Texture2D>> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, TextureType::Type::HeightMap);
            textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

// return a mesh object created from the extracted mesh data
            return Mesh::CreateMesh(vertices, indices, textures);
        };

    };


}


#endif //USUL_MODEL_H

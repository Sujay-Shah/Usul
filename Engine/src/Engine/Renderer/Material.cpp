#include "Material.h"
#include "Shader.h"
#include "Texture.h"

namespace Engine
{
    Material::Material()
    {

    }

    Material::~Material()
    {

    }

    void Material::Use()
    {
        //m_shader->Bind();
        //m_shader->SetFloat3("material.diffuse",m_materialProperty.diffuse);
       // m_shader->SetFloat3("material.specular",m_materialProperty.specular);

        for(int i=0;i<m_textures.size();++i)
        {
            m_textures[i]->Bind(uint32_t(i));
        }

    }

    const MaterialProperties &Material::GetMaterialProperties() const {
        return m_materialProperty;
    }

    Material::Material(std::vector<std::string> texturePaths,Ref<Shader> shader)
    {
        for(int i=0;i<texturePaths.size();++i)
        {
            m_textures.emplace_back(Engine::Texture2D::Create(texturePaths[i]));
        }

        //default properties
        m_materialProperty.ambient = glm::vec3(0.2f, 0.2f, 0.2f);
        m_materialProperty.diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
        m_materialProperty.specular = glm::vec3( 1.0f, 1.0f, 1.0f);
        m_materialProperty.metallic = 64.0f;

        m_shader = shader;
    }

    MaterialInstance::MaterialInstance(Material &baseMaterial)
    : m_baseMaterial(baseMaterial), m_instanceProperties(baseMaterial.GetMaterialProperties())
    {

    }

    void MaterialInstance::SetInstanceProperties() {

    }

    void MaterialInstance::Use() {

    }
};
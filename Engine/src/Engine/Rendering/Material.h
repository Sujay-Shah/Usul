#ifndef Material_h__
#define Material_h__

namespace Engine
{
    class Texture;
    class Shader;

    struct MaterialProperties
    {
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
        float metallic;
        float roughness;
    };

	class Material
	{
    public:
        Material();
        Material(std::vector<std::string> texturePaths, Ref<Shader> shader);
        virtual ~Material();

        void Use();

        const MaterialProperties & GetMaterialProperties() const;
        MaterialProperties m_materialProperty;

    private:
        std::vector<Ref<Texture>> m_textures;
        Ref<Shader> m_shader;
	};

    class MaterialInstance
    {
    public:
        MaterialInstance(Material& baseMaterial);

        void SetInstanceProperties();

        void Use();

    private:
        Material& m_baseMaterial;
        MaterialProperties m_instanceProperties;
    };


}


#endif // Material_h__
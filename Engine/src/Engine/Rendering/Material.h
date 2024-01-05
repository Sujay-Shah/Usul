#ifndef Material_h__
#define Material_h__

class Texture;
class Shader;

namespace Engine
{
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
        Material();
        virtual ~Material();
    private:
        MaterialProperties m_materialProperty;
        std::vector<Texture*> m_textures;
        Shader* m_shader;
	};
}

#endif // Material_h__
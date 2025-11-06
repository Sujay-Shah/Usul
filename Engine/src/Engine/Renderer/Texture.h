#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include <string>
#include "Engine/Core/EngineDefines.h"

namespace Engine
{
    namespace TextureType
    {
        enum Type
        {
            Diffuse,
            Specular,
            NormalMap,
            HeightMap,
            Num
        };
    }


    class Texture
    {
        public:

            /*explicit Texture(std::string path)
            :m_path(path)
            {

            }*/
            virtual ~Texture() = default;

            virtual uint32_t GetHeight() const = 0;
            virtual uint32_t GetWidth() const  = 0;

            virtual void SetData(void* data, uint32_t size) = 0;
            virtual void Bind(uint32_t slot = 0) const = 0;

            virtual uint32_t GetRendererID() const = 0;
            virtual bool operator==(const Texture& other) const = 0;
        TextureType::Type GetTextureType(){return m_type;}

        std::string m_path;
        TextureType::Type m_type;
    protected:

        uint32_t m_rendererID;

    };

    class Texture2D : public Texture
    {
        public:
            static Ref<Texture2D> Create(uint32_t width, uint32_t height);
            static Ref<Texture2D> Create(const std::string& path);


    };
}

#endif
#include "RHI/Texture.h"
#include "RHI/Renderer.h"
#include "RHI/Backends/OpenGL/TextureOpenGL.h"

namespace Engine
{
    Ref<Texture2D> Texture2D::Create(uint32_t width, uint32_t height)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:    return nullptr;
            case RendererAPI::API::OpenGL:  return std::make_shared<Texture2DOpenGL>(width, height);
        }

        return nullptr;
    }

    Ref<Texture2D> Texture2D::Create(const std::string& path)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:    return nullptr;
            case RendererAPI::API::OpenGL:  return std::make_shared<Texture2DOpenGL>(path);
        }

        return nullptr;
    }
}

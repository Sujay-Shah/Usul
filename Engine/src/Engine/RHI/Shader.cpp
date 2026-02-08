#include "RHI/Shader.h"
#include "RHI/Renderer.h"
#include "RHI/Backends/OpenGL/ShaderOpenGL.h"

namespace Engine
{
    Ref<Shader> Shader::Create(const std::string& filename)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:    return nullptr;
            case RendererAPI::API::OpenGL:  return std::make_shared<ShaderOpenGL>(filename);
        }

        return nullptr;
    }

    Ref<Shader> Shader::Create(const std::string& name, const std::string& vertexSrc, const std::string& fragSrc)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:    return nullptr;
            case RendererAPI::API::OpenGL:  return std::make_shared<ShaderOpenGL>(name, vertexSrc, fragSrc);
        }

        return nullptr;
    }

    void ShaderLibrary::Add(const std::string& name, const Ref<Shader>& shader)
    {
        if (Exists(name))
        {
            ENGINE_CORE_WARN("Shader {0} already exists in library!", name);
            return;
        }
        m_shaders[name] = shader;
    }

    void ShaderLibrary::Add(const Ref<Shader>& shader)
    {
        Add(shader->GetName(), shader);
    }

    Ref<Shader> ShaderLibrary::Load(const std::string& name)
    {
        Ref<Shader> shader = Shader::Create(name);
        Add(name, shader);
        return shader;
    }

    Ref<Shader> ShaderLibrary::Load(const std::string& name, const std::string& filepath)
    {
        Ref<Shader> shader = Shader::Create(filepath);
        Add(name, shader);
        return shader;
    }

    Ref<Shader> ShaderLibrary::Get(const std::string& name)
    {
        if (!Exists(name))
        {
            ENGINE_CORE_WARN("Shader {0} not found in library!", name);
            return nullptr;
        }
        return m_shaders[name];
    }

    bool ShaderLibrary::Exists(const std::string& name)
    {
        return m_shaders.find(name) != m_shaders.end();
    }
}

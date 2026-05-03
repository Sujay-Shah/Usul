#include "Shader.h"

#include "Engine/Core/Logging.h"
#include "Engine/Core/EngineDefines.h"

namespace Engine
{
    Ref<Shader> Shader::Create(const std::string& filename)
    {
        // RHI shader creation goes here
        return nullptr;
    }

    Ref<Shader> Shader::Create(const std::string& name, const std::string& vertexSrc, const std::string& fragSrc)
    {
        // RHI shader creation goes here
        return nullptr;
    }

    bool ShaderLibrary::Exists(const std::string& name)
    {
        return m_shaders.find(name) != m_shaders.end();
    }

    void ShaderLibrary::Add(const std::string& name, const Ref<Shader>& shader)
    {
        ENGINE_ASSERT(!Exists(name) , "Shader already exists in library!");
        m_shaders[name] = shader;
    }

    void ShaderLibrary::Add(const Ref<Shader>& shader)
    {
        std::string shaderName = shader->GetName();
        Add(shaderName, shader);
    }

    Ref<Shader> ShaderLibrary::Load(const std::string& name)
    {
        auto shader = Shader::Create(name);
        Add(shader);
        return shader;
    }

    Ref<Shader> ShaderLibrary::Get(const std::string& name)
    {
        ENGINE_CORE_ASSERT(Exists(name), "Shader does not exist in library!");
        return m_shaders[name];
    }
}

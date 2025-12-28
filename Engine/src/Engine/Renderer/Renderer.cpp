#include "Renderer.h"
#include "Renderer2D.h"
#include "Platform/OpenGL/RendererAPIOpenGL.h"
#include "Platform/Vulkan/RendererAPIVulkan.h"

namespace Engine
{
    Scope<Renderer::SceneData> Renderer::s_sceneData = std::make_unique<Renderer::SceneData>(Renderer::SceneData());
    Scope<RendererAPI> Renderer::s_rendererAPI = nullptr;

    void Renderer::Init()
    {
        #if API_VULKAN
            s_rendererAPI = std::make_unique<RendererAPIVulkan>();
        #else
            s_rendererAPI = std::make_unique<RendererAPIOpenGL>();
        #endif
        s_rendererAPI->Init();
        Renderer2D::Init();
    }

    void Renderer::BeginScene(const Camera& camera)
    {
        s_sceneData->m_viewProjectionMatrix = camera.GetViewProjectionMatrix();
    }

    void Renderer::EndScene()
    {

    }

    void Renderer::Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray, const glm::mat4& transform)
    {
        shader->Bind();
        shader->UploadUniformMat4("u_viewProjection", s_sceneData->m_viewProjectionMatrix);
        shader->UploadUniformMat4("u_transform", transform);
        
        vertexArray->Bind();
        if (vertexArray->GetIndexBuffer())
        {
            DrawIndexed(vertexArray);
        }
        else
        {
            DrawArrays(vertexArray);
        }
        
    }

    void Renderer::OnWindowResize(uint32_t width, uint32_t height)
    {
        SetViewPort(0, 0, width, height);
    }

    void Renderer::SetViewPort(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        s_rendererAPI->SetViewPort(x, y, width, height);
    }

    void Renderer::SetClearColor(const glm::vec4& color)
    {
        s_rendererAPI->SetClearColor(color);
    }

    void Renderer::Clear()
    {
        s_rendererAPI->Clear();
    }

    void Renderer::DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount)
    {
        s_rendererAPI->DrawIndexed(vertexArray, indexCount);
    }

    void Renderer::DrawArrays(const Ref<VertexArray>& vertexArray, uint32_t indexCount)
    {
        s_rendererAPI->DrawArrays(vertexArray, indexCount);
    }

    void Renderer::Cleanup()
    {
        // TODO: implement
    }
}

#include "TriangleDemo.h"

#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

TriangleDemo::TriangleDemo()
	:
	Engine::Layer("Triangle"), m_cameraController(1280.0f / 720.0f)
{
    m_triangleVA = Engine::VertexArray::Create();

    float triangleVertices[3 * 7] =
            {
                    -0.5f, -0.5f, 0.0f, 0.8f, 0.3f, 0.8f, 1.0f,
                    0.5f, -0.5f, 0.0f, 0.3f, 0.2f, 0.8f, 1.0f,
                    0.0f,  0.5f, 0.0f, 0.7f, 0.8f, 0.2f, 1.0f
            };

    Engine::Ref<Engine::VertexBuffer> triangleVB;
    triangleVB = Engine::VertexBuffer::Create(triangleVertices, sizeof(triangleVertices));
    triangleVB->SetLayout({
                                  { Engine::ShaderDataType::Float3, "a_Position"},
                                  { Engine::ShaderDataType::Float4, "a_Color"}
                          });
    m_triangleVA->AddVertexBuffer(triangleVB);

    uint32_t triangleIndices[3] = { 0, 1, 2 };
    Engine::Ref<Engine::IndexBuffer> triangleIB;
    triangleIB = Engine::IndexBuffer::Create(triangleIndices, sizeof(triangleIndices) / sizeof(uint32_t));
    m_triangleVA->SetIndexBuffer(triangleIB);

    m_shaderLibrary.Add(Engine::Shader::Create("shaders/SingleColor.glsl"));
}

void TriangleDemo::OnAttach()
{
    
}

void TriangleDemo::OnDetach()
{
}

void TriangleDemo::OnUpdate(const Engine::Timestep& ts)
{
    m_cameraController.OnUpdate(ts);

    Engine::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1 });
    Engine::RenderCommand::Clear();

    Engine::Renderer::BeginScene(m_cameraController.GetCamera());

    // triangle
    auto m_shader = m_shaderLibrary.Get("SingleColor");
    m_shader->UploadUniformFloat3("u_color", triangleColor);
    Engine::Renderer::Submit(m_shader, m_triangleVA);
    //Engine::Renderer2D::DrawQuad({ 0.0f, -0.5f }, { 0.5f, 0.75f }, { 0.2f, 0.3f, 0.8f, 1.0f });
    //Engine::Renderer2D::DrawTexture({ 0.2f, 0.5f, -0.1f }, { 10.0f, 10.0f }, glm::vec4(1.0f), 10.0f, m_texture);
    //Engine::Renderer2D::DrawTexture(trianglePos, scale, triangleColor, 1.0f, m_texture2);

    Engine::Renderer::EndScene();
}

void TriangleDemo::OnImGuiRender()
{
    ImGui::Begin("Settings");
    ImGui::ColorEdit4("Triangle Color", glm::value_ptr(triangleColor));

    ImGui::End();
}

void TriangleDemo::OnEvent(Engine::Event& e)
{
    m_cameraController.OnEvent(e);
}

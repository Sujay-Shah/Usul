//
// Created by snsha on 2024-01-10.
//

#include "MaterialExample.h"
#include "imgui.h"
#include <glm/gtc/type_ptr.hpp>
#include "Platform/GLFW/TimeGLFW.h"

MaterialExample::MaterialExample()
: Layer("MaterialExample"),m_cameraController(1280.0f / 720.0f), m_cameraPosition(-5.0,0.0f,0.0f)
{

    std::string path = "../Editor/assets/";
    const float sideLength = 0.5f;
    float vertices[] = {
            // positions          // normals           // texture coords
            // Positions          // Normals           // Texture Coordinates
            -sideLength, -sideLength, -sideLength,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f,
            sideLength, -sideLength, -sideLength,  0.0f, 0.0f, -1.0f,  1.0f, 0.0f,
            sideLength,  sideLength, -sideLength,  0.0f, 0.0f, -1.0f,  1.0f, 1.0f,
            -sideLength,  sideLength, -sideLength,  0.0f, 0.0f, -1.0f,  0.0f, 1.0f,

            -sideLength, -sideLength,  sideLength,  0.0f, 0.0f,  1.0f,  0.0f, 0.0f,
            sideLength, -sideLength,  sideLength,  0.0f, 0.0f,  1.0f,  1.0f, 0.0f,
            sideLength,  sideLength,  sideLength,  0.0f, 0.0f,  1.0f,  1.0f, 1.0f,
            -sideLength,  sideLength,  sideLength,  0.0f, 0.0f,  1.0f,  0.0f, 1.0f,
    };

    unsigned int indices[] = {
            0, 1, 2, 2, 3, 0,   // Front face
            4, 5, 6, 6, 7, 4,   // Back face
            0, 3, 7, 7, 4, 0,   // Left face
            1, 2, 6, 6, 5, 1,   // Right face
            0, 1, 5, 5, 4, 0,   // Bottom face
            2, 3, 7, 7, 6, 2    // Top face
    };

    //normal cube
    m_cubeVA = Engine::VertexArray::Create();
    Engine::Ref<Engine::VertexBuffer> cubeVB;
    cubeVB = Engine::VertexBuffer::Create(vertices, sizeof(vertices));
    cubeVB->SetLayout({
                              { Engine::ShaderDataType::Float3, "aPos"},
                              { Engine::ShaderDataType::Float3, "aNormal"},
                              { Engine::ShaderDataType::Float2, "aTexCoords"}
                      });
    m_cubeVA->AddVertexBuffer(cubeVB);

    Engine::Ref<Engine::IndexBuffer> cubeIB;
    cubeIB = Engine::IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));
    m_cubeVA->SetIndexBuffer(cubeIB);

    //light cube uses same coords, so just set buffers
    m_lightCubeVA = Engine::VertexArray::Create();
    Engine::Ref<Engine::VertexBuffer> lightcubeVB;
    lightcubeVB = Engine::VertexBuffer::Create(vertices, sizeof(vertices));
    lightcubeVB->SetLayout({
                                   { Engine::ShaderDataType::Float3, "aPos"},
                                   { Engine::ShaderDataType::Float3, "aNormal"},
                                   { Engine::ShaderDataType::Float2, "aTexCoords"}
                           });
    m_lightCubeVA->AddVertexBuffer(lightcubeVB);
    Engine::Ref<Engine::IndexBuffer> lightcubeIB;
    lightcubeIB = Engine::IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));
    m_lightCubeVA->SetIndexBuffer(lightcubeIB);


    m_shaderLibrary.Add(Engine::Shader::Create(path + "shaders/LightMaps.glsl"));
    m_shaderLibrary.Add(Engine::Shader::Create(path+"shaders/CubeLight.glsl"));

    m_cameraController.SetPos(m_cameraPosition);
    m_cameraController.SetRotation(-3.05,5.0,0.0f);

    std::vector<std::string> textures {path+"textures/container2.png",path+"textures/container2_specular.png"};
    m_material = std::make_shared<Engine::Material>(textures,m_shaderLibrary.Get("LightMaps"));


}

MaterialExample::~MaterialExample() {

}

void MaterialExample::OnUpdate(const Engine::Timestep &ts) {

    m_cameraController.OnUpdate(ts);

    Engine::RenderCommand::SetClearColor({ 0.0f, 0.0f, 0.0f, 1 });
    Engine::RenderCommand::Clear();

    Engine::Renderer::BeginScene(m_cameraController.GetCamera());

    m_lightPos.x = 2.0f * sinf(Engine::GetTime());
    m_lightPos.z = 1.5f * cosf(Engine::GetTime());

    auto lightShader = m_shaderLibrary.Get("LightMaps");
    lightShader->Bind();

    lightShader->UploadUniformFloat3("light.ambient", m_material->m_materialProperty.ambient);
    lightShader->UploadUniformFloat3("light.diffuse", m_material->m_materialProperty.diffuse);
    lightShader->UploadUniformFloat3("light.specular", m_material->m_materialProperty.specular);

    lightShader->UploadUniformFloat3("light.position", m_lightPos);// glm::vec3(m_lightPos.x, m_lightPos.y, m_lightPos.z));
    lightShader->UploadUniformFloat3("viewPos", m_cameraPosition);

    lightShader->SetFloat("material.shininess",m_material->m_materialProperty.metallic);

    lightShader->UploadUniformMat4("projection", m_cameraController.GetCamera().GetProjectionMatrix());
    lightShader->UploadUniformMat4("view", m_cameraController.GetCamera().GetViewMatrix());

    m_material->Use();

    lightShader->UploadUniformMat4("model", glm::mat4(1.0f));
    Engine::Renderer::Submit(lightShader, m_cubeVA);

    auto LightcubeShader = m_shaderLibrary.Get("CubeLight");
    LightcubeShader->Bind();

    glm::mat4 model = glm::mat4(1.0f);



    model = glm::translate(model, m_lightPos);
    model = glm::scale(model, glm::vec3(0.5f)); // a smaller cube

    LightcubeShader->UploadUniformMat4("model", model);
    LightcubeShader->UploadUniformMat4("projection", m_cameraController.GetCamera().GetProjectionMatrix());
    LightcubeShader->UploadUniformMat4("view", m_cameraController.GetCamera().GetViewMatrix());
    Engine::Renderer::Submit(LightcubeShader, m_lightCubeVA);

}

void MaterialExample::OnAttach() {

}

void MaterialExample::OnDetach() {

}

void MaterialExample::OnEvent(Engine::Event &e) {
    m_cameraController.OnEvent(e);
}

void MaterialExample::OnImGuiRender()
{
    ImGui::Begin("Setting");
    ImGui::SliderFloat3("Ambient", glm::value_ptr(m_material->m_materialProperty.ambient), -1.0f, 1.0f);
    ImGui::SliderFloat3("Diffuse", glm::value_ptr(m_material->m_materialProperty.diffuse), -1.0f, 1.0f);
    ImGui::SliderFloat3("Specular", glm::value_ptr(m_material->m_materialProperty.specular), -1.0f, 1.0f);
    ImGui::SliderFloat3("Light pos", glm::value_ptr(m_lightPos), -10.0f, 10.0f);
    ImGui::SliderFloat("Metallic", &m_material->m_materialProperty.metallic, 0.0f, 100.0f);
    ImGui::End();
}


